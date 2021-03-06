#ifndef METASHELL_FRAME_HPP
#define METASHELL_FRAME_HPP

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

#include <metashell/data/event_kind.hpp>
#include <metashell/data/file_location.hpp>
#include <metashell/data/metaprogram_node.hpp>

#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace metashell
{
  namespace data
  {
    class frame : boost::equality_comparable<frame>
    {
    public:
      frame() = default;

      frame(const metaprogram_node& node_,
            const file_location& source_location_);

      frame(const metaprogram_node& node_,
            const file_location& source_location_,
            const file_location& point_of_event_,
            event_kind kind_,
            boost::optional<double> time_taken = boost::none,
            boost::optional<double> time_taken_ratio = boost::none);

      const data::metaprogram_node& node() const;
      const file_location& source_location() const;

      bool is_full() const;
      bool is_profiled() const;

      // precondition: is_full()
      event_kind kind() const;
      const file_location& point_of_event() const;

      // precondition: is_profiled()
      double time_taken() const;
      double time_taken_ratio() const;

    private:
      metaprogram_node _node;
      file_location _source_location;
      boost::optional<file_location> _point_of_event;
      boost::optional<data::event_kind> _kind;
      boost::optional<double> _time_taken;
      boost::optional<double> _time_taken_ratio;
    };

    std::ostream& operator<<(std::ostream& o_, const frame& f_);
    bool operator==(const frame& a_, const frame& b_);
  }
}

#endif
