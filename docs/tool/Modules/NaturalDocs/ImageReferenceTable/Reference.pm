###############################################################################
#
#   Package: NaturalDocs::ImageReferenceTable::Reference
#
###############################################################################
#
#   A class for references being tracked in <NaturalDocs::SourceDB>.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2008 Greg Valure
# Natural Docs is licensed under the GPL

use strict;
use integer;


package NaturalDocs::ImageReferenceTable::Reference;

use base 'NaturalDocs::SourceDB::Item';


use NaturalDocs::DefineMembers 'TARGET', 'Target()', 'SetTarget()',
                                                 'NEEDS_REBUILD', 'NeedsRebuild()', 'SetNeedsRebuild()';


#
#   Variables: Members
#
#   The following constants are indexes into the object array.
#
#   TARGET - The image <FileName> this reference resolves to, or undef if none.
#


#
#   Functions: Member Functions
#
#   Target - Returns the image <FileName> this reference resolves to, or undef if none.
#   SetTarget - Replaces the image <FileName> this reference resolves to, or undef if none.
#


1;
