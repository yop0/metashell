// Metashell - Interactive C++ template metaprogramming shell
// Copyright (C) 2015, Abel Sinkovics (abel@sinkovics.hu)
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

#include <metashell_system_test/cpp_code.hpp>
#include <metashell_system_test/json_generator.hpp>
#include <metashell_system_test/run_metashell.hpp>

#include <just/test.hpp>

using namespace metashell_system_test;

JUST_TEST_CASE(test_getting_defined_macro)
{
  const auto r =
    run_metashell(
      {
        command("#define FOO bar"),
        command("#msh macros")
      }
    );

  auto i = r.begin() + 2;

  std::string macros;
  JUST_ASSERT_EQUAL(cpp_code(&macros), *i);
  JUST_ASSERT(macros.find("#define FOO bar") != std::string::npos);
}

JUST_TEST_CASE(test_getting_defined_macro_name)
{
  const auto r =
    run_metashell(
      {
        command("#define FOO bar"),
        command("#msh macro names")
      }
    );

  auto i = r.begin() + 2;

  std::string names;
  JUST_ASSERT_EQUAL(cpp_code(&names), *i);
  JUST_ASSERT(names.find("FOO") != std::string::npos);
  JUST_ASSERT(names.find("#define") == std::string::npos);
}

