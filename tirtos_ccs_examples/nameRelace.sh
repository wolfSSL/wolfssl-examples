# nameReplace.sh
# Copyright (C) 2015 wolfSSL Inc.
#
# This file is a script developed and used during re-branding from CyaSSL to wolfSSL
#
# @author: Kaleb
#
# wolfSSL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# wolfSSL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
#


####################### Unix/Linux/Ubuntu ##############################
#find . -name '*' -type f -exec sed -i 's/old-word/new-word/g' {} \;
#######################################################################

################################ MAC ###################################
#grep -ilr 'old-word' * | xargs -I@ sed -i '' 's/old-word/new-word/g' @
########################################################################

#grep -ilr 'This file is part of wolfSSL. (formerly known as CyaSSL)' * | xargs -I@ sed -i '' 's/This file is part of wolfSSL. (formerly known as CyaSSL)/This file is part of wolfSSL. (formerly known as CyaSSL)/g' @
#grep -ilr 'wolfSSL is free' * | xargs -I@ sed -i '' 's/wolfSSL is free/wolfSSL is free/g' @
#grep -ilr 'wolfSSL is distributed' * | xargs -I@ sed -i '' 's/wolfSSL is distributed/wolfSSL is distributed/g' @
#grep -ilr 'libwolfssl' * | xargs -I@ sed -i '' 's/libwolfssl/libwolfssl/g' @

#grep -ilr '2014 wolfSSL' * | xargs -I@ sed -i '' 's/2014 wolfSSL/2015 wolfSSL/g' @

#grep -ilr '2013 wolfSSL' * | xargs -I@ sed -i '' 's/2013 wolfSSL/2015 wolfSSL/g' @


