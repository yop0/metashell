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

#include <metashell/data/command.hpp>

#include <just/test.hpp>

using namespace metashell::data;

JUST_TEST_CASE(test_formatting_empty_token_list)
{
  const command cmd("");
  JUST_ASSERT_EQUAL("", tokens_to_string(cmd.begin(), cmd.end()));
}

JUST_TEST_CASE(test_formatting_one_token)
{
  const command cmd("int hello");

  JUST_ASSERT_EQUAL("int", tokens_to_string(cmd.begin(), skip(cmd.begin())));
}

JUST_TEST_CASE(test_formatting_more_tokens)
{
  const command cmd("int hello");

  JUST_ASSERT_EQUAL("int hello", tokens_to_string(cmd.begin(), cmd.end()));
}

