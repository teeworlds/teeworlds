###############################################################################
#
#   Package: NaturalDocs::Version
#
###############################################################################
#
#   A package for handling version information.  What?  That's right.  Although it should be easy and obvious, version numbers
#   need to be dealt with in a variety of formats, plus there's compatibility with older releases which handled it differently.  I
#   wanted to centralize the code after it started getting complicated.  So there ya go.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL

use strict;
use integer;

package NaturalDocs::Version;

#
#   About: Format
#
#   Version numbers are represented as major.minor.  Major is from 0 to 255, and minor can have one or two digits.  Minor is
#   interpreted as a decimal, so 1.25 is less than 1.3.
#

# Group: Functions


#
#   Function: FromString
#
#   Converts a version string to a <VersionInt>.
#
sub FromString #(string)
    {
    my ($self, $string) = @_;

    if ($string eq '1')
        {
        return 91;  # 0.91
        }
    else
        {
        $string =~ /^(\d+)\.(\d+)$/;
        my ($major, $minor) = ($1, $2);

        if (length $minor == 1)
            {  $minor *= 10;  };

        return ($major << 8) | $minor;
        };
    };

#
#   Function: FromBinaryFile
#
#   Retrieves a <VersionInt> from a binary file.
#
#   Parameters:
#
#       fileHandle - The handle of the file to read it from.  It should be at the correct location.
#
#   Returns:
#
#       The <VersionInt>.
#
sub FromBinaryFile #(fileHandle)
    {
    my ($self, $fileHandle) = @_;

    my $version;
    read($fileHandle, $version, 2);

    # A big-endian UInt16 as a shortcut to the integer format.
    return unpack('n', $version);
    };

#
#   Function: FromTextFile
#
#   Retrieves a <VersionInt> from a text file.
#
#   Parameters:
#
#       fileHandle - The handle of the file to read it from.  It should be at the correct location.
#
#   Returns:
#
#       The <VersionInt>.
#
sub FromTextFile #(fileHandle)
    {
    my ($self, $fileHandle) = @_;

    my $version = <$fileHandle>;
    ::XChomp(\$version);

    return $self->FromString($version);
    };


#
#   Function: ToString
#
#   Converts a <VersionInt> to a string.
#
sub ToString #(integer)
    {
    my ($self, $integer) = @_;

    my $major = $integer >> 8;
    my $minor = $integer & 0x00FF;

    if ($minor % 10 == 0)
        {  $minor /= 10;  }
    elsif ($minor < 10)
        {  $minor = '0' . $minor;  };

    return $major . '.' . $minor;
    };


#
#   Function: ToTextFile
#
#   Writes a <VersionInt> to a text file.
#
#   Parameters:
#
#       fileHandle - The handle of the file to write it to.  It should be at the correct location.
#       version - The <VersionInt> to write.
#
sub ToTextFile #(fileHandle, version)
    {
    my ($self, $fileHandle, $version) = @_;

    print $fileHandle $self->ToString($version) . "\n";
    };


#
#   Function: ToBinaryFile
#
#   Writes a <VersionInt> to a binary file.
#
#   Parameters:
#
#       fileHandle - The handle of the file to write it to.  It should be at the correct location.
#       version - The <VersionInt> to write.
#
sub ToBinaryFile #(fileHandle, version)
    {
    my ($self, $fileHandle, $version) = @_;

    # Big-endian UInt16 as a shortcut to the binary format.
    print $fileHandle pack('n', $version);
    };



###############################################################################
# Group: Implementation

#
#   About: String Format
#
#   String versions are normally in the common major.minor format, with the exception of "1".
#
#   If the string is "1" and not "1.0", it's represents releases 0.85 through 0.91, since those had a separate version number for data
#   files.  We switched to using the app version number in 0.95.  This issue does not apply to binary data files since they came
#   after 0.95.
#
#   Text files with "1" as the version will be interpreted as 0.91, since this should not cause compatibility problems.  The only
#   file format changes between 0.85 and 0.91 were to <PreviousMenuState.nd>, which didn't exist in 0.85 and didn't change
#   between 0.9 and 0.91, and <Menu.txt>, which only changed in 0.9 to add index entries.
#

#
#   About: Integer Format
#
#   <VersionInts> are 16-bit unsigned values.  The major version is the high-order byte, and the minor the low-order byte.
#   The minor is always stored with two decimals, so 0.9 would be stored as 0 and 90.
#

#
#   About: Binary File Format
#
#   In binary files, versions are two 8-bit unsigned values, appearing major then minor.  The minor is always stored with two
#   decimals, so 0.9 would be stored as 0 and 90.  It's in the <Integer Format> if interpreted as a _big-endian_ 16-bit value.
#

#
#   About: Text File Format
#
#   In text files, versions are the <String Format> followed by a native line break.
#


1;
