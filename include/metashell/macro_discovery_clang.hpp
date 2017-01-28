#ifndef METASHELL_MACRO_DISCOVERY_CLANG_HPP
#define METASHELL_MACRO_DISCOVERY_CLANG_HPP

// Metashell - Interactive C++ template metaprogramming shell
// Copyright (C) 2016, Abel Sinkovics (abel@sinkovics.hu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <metashell/iface/macro_discovery.hpp>

#include <metashell/clang_binary.hpp>

namespace metashell
{
  class macro_discovery_clang : public iface::macro_discovery
  {
  public:
    explicit macro_discovery_clang(clang_binary clang_binary_);

    virtual data::cpp_code macros(const iface::environment& env_) override;

  private:
    clang_binary _clang_binary;
  };
}

#endif
