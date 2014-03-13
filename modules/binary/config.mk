#
#   Copyright 2014 Fabian "Praetorian" Kürten
#
#   This file is part of tyrant-genetics.
#   The tyrant-genetics library is free software: you can redistribute
#   it and/or modify it under the terms of the GNU General Public
#   Licenseas published by the Free Software Foundation, exactly version
#   3 ofthe License.
#
#   The tyrant-genetics library is distributed in the hope that it will be
#   useful but WITHOUT ANY WARRANTY; without even the implied warranty
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program (COPYING.GPL).  If not, see
#   <http://www.gnu.org/licenses/>.
#

# Configuration section
MODULE_NAME=tyrantGenetic
MODULE_MAJOR_VERSION=1
MODULE_MINOR_VERSION=0
DEPENDENCIES=core cache mutator
STATIC_LIBRARIES:=
DYNAMIC_LIBRARIES:=
LIBRARIES:=boost_program_options boost_regex
CXX_STANDARD=c++11
