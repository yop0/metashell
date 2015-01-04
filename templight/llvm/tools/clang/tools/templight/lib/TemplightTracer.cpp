//===- TemplightTracer.cpp ------ Clang Templight Profiler / Tracer -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TemplightTracer.h"

#include "TemplightProtobufWriter.h"
#include "PrintableTemplightEntries.h"
#include "TemplightEntryPrinter.h"

// FIXME: Eventually these extra writers will be removed from templight program (only appear in converter).
#include "TemplightExtraWriters.h"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Sema/ActiveTemplateInst.h>
#include <clang/Sema/Sema.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/YAMLTraits.h>

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace clang {

namespace {


struct RawTemplightTraceEntry {
  bool IsTemplateBegin;
  std::size_t ParentBeginIdx;
  ActiveTemplateInstantiation::InstantiationKind InstantiationKind;
  Decl *Entity;
  SourceLocation PointOfInstantiation;
  double TimeStamp;
  std::uint64_t MemoryUsage;
  
  static const std::size_t invalid_parent = ~std::size_t(0);
  
  RawTemplightTraceEntry() : IsTemplateBegin(true), ParentBeginIdx(invalid_parent),
    InstantiationKind(ActiveTemplateInstantiation::TemplateInstantiation),
    Entity(0), TimeStamp(0.0), MemoryUsage(0) { };
};

PrintableTemplightEntryBegin rawToPrintableBegin(const Sema &TheSema, const RawTemplightTraceEntry& Entry) {
  PrintableTemplightEntryBegin Ret;
  
  Ret.InstantiationKind = Entry.InstantiationKind;
  
  NamedDecl *NamedTemplate = dyn_cast_or_null<NamedDecl>(Entry.Entity);
  if (NamedTemplate) {
    llvm::raw_string_ostream OS(Ret.Name);
    NamedTemplate->getNameForDiagnostic(OS, TheSema.getLangOpts(), true);
  }
  
  PresumedLoc Loc = TheSema.getSourceManager().getPresumedLoc(Entry.PointOfInstantiation);
  if (!Loc.isInvalid()) {
    Ret.FileName = Loc.getFilename();
    Ret.Line = Loc.getLine();
    Ret.Column = Loc.getColumn();
  } else {
    Ret.FileName = "";
    Ret.Line = 0;
    Ret.Column = 0;
  }
  
  Ret.TimeStamp = Entry.TimeStamp;
  Ret.MemoryUsage = Entry.MemoryUsage;
  
  if (Entry.Entity) {
    PresumedLoc Loc = TheSema.getSourceManager().getPresumedLoc(Entry.Entity->getLocation());
    if (!Loc.isInvalid()) {
      Ret.TempOri_FileName = Loc.getFilename();
      Ret.TempOri_Line = Loc.getLine();
      Ret.TempOri_Column = Loc.getColumn();
    } else {
      Ret.TempOri_FileName = "";
      Ret.TempOri_Line = 0;
      Ret.TempOri_Column = 0;
    }
  }
  
  return Ret;
}

PrintableTemplightEntryEnd rawToPrintableEnd(const Sema &TheSema, const RawTemplightTraceEntry& Entry) {
  return {Entry.TimeStamp, Entry.MemoryUsage};
}

} // unnamed namespace



class TemplightTracer::TracePrinter : public TemplightEntryPrinter {
public:
  
  void skipRawEntry(const RawTemplightTraceEntry &Entry) {
    skipEntry();
  }
  
  bool shouldIgnoreRawEntry(const RawTemplightTraceEntry &Entry) {
    
    // Avoid some duplication of memoization entries:
    if ( ( Entry.InstantiationKind == ActiveTemplateInstantiation::Memoization ) &&
          LastClosedMemoization && ( LastClosedMemoization == Entry.Entity ) ) {
      return true;
    }
    
    // if we have an end entry, we must ensure it corresponds to the current begin entry:
    //  these checks are a bit redundant and overly cautious, but better safe than sorry when sanitizing.
    if ( (!Entry.IsTemplateBegin) &&
         ( ( TraceEntries.empty() ) || 
           ( CurrentParentBegin == RawTemplightTraceEntry::invalid_parent ) || 
           ( CurrentParentBegin >= TraceEntries.size() ) ||
           !( ( TraceEntries[CurrentParentBegin].InstantiationKind == Entry.InstantiationKind ) &&
              ( TraceEntries[CurrentParentBegin].Entity == Entry.Entity ) ) ) ) {
      return true; // ignore end entries that don't match the current begin entry.
    }
    
    return false;
  };
  
  void printOrSkipEntry(RawTemplightTraceEntry &Entry) {
    if ( IgnoreSystemFlag && TheSema.getSourceManager()
                                .isInSystemHeader(Entry.PointOfInstantiation) ) {
      skipRawEntry(Entry); // recursively skip all entries until end of this one.
    } else {
      if ( Entry.IsTemplateBegin ) {
        printEntry(rawToPrintableBegin(TheSema, Entry));
      } else {
        printEntry(rawToPrintableEnd(TheSema, Entry));
      }
    }
  };
  
  void printCachedRawEntries() {
    for(std::vector<RawTemplightTraceEntry>::iterator it = TraceEntries.begin();
        it != TraceEntries.end(); ++it)
      printOrSkipEntry(*it);
    TraceEntries.clear();
    CurrentParentBegin = RawTemplightTraceEntry::invalid_parent;
  };
  
  void printRawEntry(RawTemplightTraceEntry Entry, bool inSafeMode = false) {
    if ( shouldIgnoreRawEntry(Entry) )
      return;
    
    if ( inSafeMode )
      printOrSkipEntry(Entry);
    
    // Always maintain a stack of cached trace entries such that the sanity of the traces can be enforced.
    if ( Entry.IsTemplateBegin ) {
      Entry.ParentBeginIdx = CurrentParentBegin;
      CurrentParentBegin = TraceEntries.size();
    } else { // note: this point should not be reached if CurrentParentBegin is not valid.
      Entry.ParentBeginIdx = TraceEntries[CurrentParentBegin].ParentBeginIdx;
      CurrentParentBegin = Entry.ParentBeginIdx;
    };
    TraceEntries.push_back(Entry); 
    
    if ( Entry.IsTemplateBegin )
      LastClosedMemoization = nullptr;
    if ( !Entry.IsTemplateBegin && 
         ( Entry.InstantiationKind == ActiveTemplateInstantiation::Memoization ) )
      LastClosedMemoization = Entry.Entity;
    
    if ( !Entry.IsTemplateBegin &&
         ( Entry.InstantiationKind == TraceEntries.front().InstantiationKind ) &&
         ( Entry.Entity == TraceEntries.front().Entity ) ) {  // did we reach the end of the top-level begin entry?
      if ( !inSafeMode ) { // if not in safe-mode, print out the cached entries.
        printCachedRawEntries();
      } else { // if in safe-mode, simply clear the cached entries.
        TraceEntries.clear();
        CurrentParentBegin = RawTemplightTraceEntry::invalid_parent;
      }
    }
  };
  
  void startTrace() {
    // get the source name from the source manager:
    FileID fileID = TheSema.getSourceManager().getMainFileID();
    std::string src_name =
      TheSema.getSourceManager().getFileEntryForID(fileID)->getName();
    initialize(src_name);
  };
  
  void endTrace() {
    printCachedRawEntries();
    finalize();
  };
  
  TracePrinter(const Sema &aSema, const std::string &Output, bool IgnoreSystem = false) : 
               TemplightEntryPrinter(Output), TheSema(aSema), 
               LastClosedMemoization(nullptr), 
               CurrentParentBegin(RawTemplightTraceEntry::invalid_parent), 
               IgnoreSystemFlag(IgnoreSystem) { };
  
  ~TracePrinter() { };
  
  const Sema &TheSema;
  
  std::vector<RawTemplightTraceEntry> TraceEntries;
  Decl* LastClosedMemoization;
  std::size_t CurrentParentBegin;
  
  unsigned IgnoreSystemFlag : 1;
  
};


void TemplightTracer::atTemplateBeginImpl(const Sema &TheSema, 
                          const ActiveTemplateInstantiation& Inst) {
  if ( !Printer )
    return;
  
  RawTemplightTraceEntry Entry;
  
  Entry.IsTemplateBegin = true;
  Entry.InstantiationKind = Inst.Kind;
  Entry.Entity = Inst.Entity;
  Entry.PointOfInstantiation = Inst.PointOfInstantiation;
  
  // NOTE: Use this function because it produces time since start of process.
  llvm::sys::TimeValue now(0,0), user(0,0), sys(0,0);
  llvm::sys::Process::GetTimeUsage(now, user, sys);
  if(user.seconds() != 0 && user.nanoseconds() != 0)
    now = user;
  
  Entry.TimeStamp = now.seconds() + now.nanoseconds() / 1000000000.0;
  Entry.MemoryUsage = (MemoryFlag ? llvm::sys::Process::GetMallocUsage() : 0);
  
  Printer->printRawEntry(Entry, SafeModeFlag);
}

void TemplightTracer::atTemplateEndImpl(const Sema &TheSema, 
                          const ActiveTemplateInstantiation& Inst) {
  if ( !Printer )
    return;
  
  RawTemplightTraceEntry Entry;
  
  Entry.IsTemplateBegin = false;
  Entry.InstantiationKind = Inst.Kind;
  Entry.Entity = Inst.Entity;
  
  // NOTE: Use this function because it produces time since start of process.
  llvm::sys::TimeValue now(0,0), user(0,0), sys(0,0);
  llvm::sys::Process::GetTimeUsage(now, user, sys);
  if(user.seconds() != 0 && user.nanoseconds() != 0)
    now = user;
  
  Entry.TimeStamp = now.seconds() + now.nanoseconds() / 1000000000.0;
  Entry.MemoryUsage = (MemoryFlag ? llvm::sys::Process::GetMallocUsage() : 0);
  
  Printer->printRawEntry(Entry, SafeModeFlag);
}

TemplightTracer::TemplightTracer(const Sema &TheSema, 
                                 std::string Output, 
                                 const std::string& Format,
                                 bool Memory, bool Safemode, 
                                 bool IgnoreSystem, 
                                 bool TraceTemplateOrigins) :
                                 MemoryFlag(Memory),
                                 SafeModeFlag(Safemode),
                                 TraceTemplateOriginsFlag(TraceTemplateOrigins) {
  
  Printer.reset(new TemplightTracer::TracePrinter(TheSema, Output, IgnoreSystem));
  
  if ( !Printer->getTraceStream() ) {
    llvm::errs() << "Error: [Templight-Tracer] Failed to create template trace file!";
    Printer.reset();
    llvm::errs() << "Note: [Templight] Template trace has been disabled.";
    return;
  }
  
  if ( ( Format.empty() ) || ( Format == "protobuf" ) ) {
    Printer->takeWriter(new clang::TemplightProtobufWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "yaml" ) {
    Printer->takeWriter(new clang::TemplightYamlWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "xml" ) {
    Printer->takeWriter(new clang::TemplightXmlWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "text" ) {
    Printer->takeWriter(new clang::TemplightTextWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "graphml" ) {
    Printer->takeWriter(new clang::TemplightGraphMLWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "graphviz" ) {
    Printer->takeWriter(new clang::TemplightGraphVizWriter(*Printer->getTraceStream()));
  } else 
  if ( Format == "nestedxml" ) {
    Printer->takeWriter(new clang::TemplightNestedXMLWriter(*Printer->getTraceStream()));
  } else 
  {
    llvm::errs() << "Error: [Templight-Tracer] Unrecognized template trace format:" << Format << "\n";
    Printer.reset();
    llvm::errs() << "Note: [Templight] Template trace has been disabled.\n";
    return;
  }
  
}

TemplightTracer::~TemplightTracer() { 
  // must be defined here due to TracePrinter being incomplete in header.
}

void TemplightTracer::initializeImpl(const Sema &) {
  if ( Printer )
    Printer->startTrace();
}

void TemplightTracer::finalizeImpl(const Sema &) {
  if ( Printer )
    Printer->endTrace();
}

void TemplightTracer::readBlacklists(const std::string& BLFilename) {
  if ( Printer )
    Printer->readBlacklists(BLFilename);
}


} // namespace clang

