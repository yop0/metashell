#ifndef METASHELL_PRAGMA_SWITCH_HPP
#define METASHELL_PRAGMA_SWITCH_HPP

// Metashell - Interactive C++ template metaprogramming shell
// Copyright (C) 2014, Abel Sinkovics (abel@sinkovics.hu)
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

#include <metashell/iface/pragma_handler.hpp>
#include <metashell/iface/displayer.hpp>

#include <string>
#include <functional>

namespace metashell
{
  class shell;

  class pragma_switch : public iface::pragma_handler
  {
  public:
    pragma_switch(
      const std::string& name_,
      const std::function<bool()>& query_,
      const std::function<void(bool)>& update_
    );

    virtual iface::pragma_handler* clone() const override;

    virtual std::string arguments() const override;
    virtual std::string description() const override;

    virtual void run(
      const data::command::iterator& args_begin_,
      const data::command::iterator& args_end_,
      iface::displayer& displayer_
    ) const override;
  private:
    std::function<bool()> _query;
    std::function<void(bool)> _update;
    std::string _name;
  };
}

#endif

