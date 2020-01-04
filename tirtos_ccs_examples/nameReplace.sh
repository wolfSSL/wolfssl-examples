# nameReplace.sh
# Copyright (C) 2020 wolfSSL Inc.
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

#find . -name '*' -type f -exec sed -i 's/wolf/wolf/g' {} \;
#find . -name '*' -type f -exec sed -i 's/wolf/wolf/g' {} \;
#find . -name '*' -type f -exec sed -i 's/WOLF/WOLF/g' {} \;
#find . -name '*' -type f -exec sed -i 's/wolf/wolf/g' {} \;
#find . -name '*' -type f -exec sed -i 's/wolf/wolf/g' {} \;
#find . -name '*' -type f -exec sed -i 's/CTao/wc_/g' {} \;
#find . -name '*' -type f -exec sed -i 's/CTAO/WOLF/g' {} \;

################################ MAC ###################################
#grep -ilr 'old-word' * | xargs -I@ sed -i '' 's/old-word/new-word/g' @
########################################################################

#grep -ilr 'cya' * | xargs -I@ sed -i '' 's/cya/wolf/g' @
#grep -ilr 'Cya' * | xargs -I@ sed -i '' 's/Cya/wolf/g' @
#grep -ilr 'CYA' * | xargs -I@ sed -i '' 's/CYA/WOLF/g' @
#grep -ilr 'ctao' * | xargs -I@ sed -i '' 's/ctao/wolf/g' @
#grep -ilr 'Ctao' * | xargs -I@ sed -i '' 's/Ctao/wolf/g' @
#grep -ilr 'CTao' * | xargs -I@ sed -i '' 's/CTao/wc_/g' @
#grep -ilr 'CTAO' * | xargs -I@ sed -i '' 's/CTAO/WOLF/g' @





#grep -ilr 'This file is part of wolfSSL. (formerly known as wolfSSL)' * | xargs -I@ sed -i '' 's/This file is part of wolfSSL. (formerly known as wolfSSL)/This file is part of wolfSSL. (formerly known as wolfSSL)/g' @
#grep -ilr 'wolfSSL is free' * | xargs -I@ sed -i '' 's/wolfSSL is free/wolfSSL is free/g' @
#grep -ilr 'wolfSSL is distributed' * | xargs -I@ sed -i '' 's/wolfSSL is distributed/wolfSSL is distributed/g' @
#grep -ilr 'libwolfssl' * | xargs -I@ sed -i '' 's/libwolfssl/libwolfssl/g' @

#grep -ilr '2014 wolfSSL' * | xargs -I@ sed -i '' 's/2014 wolfSSL/2015 wolfSSL/g' @

#grep -ilr '2013 wolfSSL' * | xargs -I@ sed -i '' 's/2013 wolfSSL/2015 wolfSSL/g' @


