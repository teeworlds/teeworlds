###############################################################################
#
#   Package: NaturalDocs::ReferenceString
#
###############################################################################
#
#   A package to manage <ReferenceString> handling throughout the program.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL

use strict;
use integer;

package NaturalDocs::ReferenceString;

use vars '@ISA', '@EXPORT';
@ISA = 'Exporter';
@EXPORT = ( 'BINARYREF_NOTYPE', 'BINARYREF_NORESOLVINGFLAGS' );


#
#   Constants: Binary Format Flags
#
#   These flags can be combined to specify the format when using <ToBinaryFile()> and <FromBinaryFile()>.  All are exported
#   by default.
#
#   BINARYREF_NOTYPE - Do not include the <ReferenceType>.
#   BINARYREF_NORESOLVEFLAGS - Do not include the <Resolving Flags>.
#
use constant BINARYREF_NOTYPE => 0x01;
use constant BINARYREF_NORESOLVINGFLAGS => 0x02;


#
#
#   Function: MakeFrom
#
#   Encodes the passed information as a <ReferenceString>.  The format of the string should be treated as opaque.  However, the
#   characteristic you can rely on is that the same string will always be made from the same parameters, and thus it's suitable
#   for comparison and use as hash keys.
#
#   Parameters:
#
#       type - The <ReferenceType>.
#       symbol - The <SymbolString> of the reference.
#       scope - The scope <SymbolString> the reference appears in, or undef if none.
#       using - An arrayref of scope <SymbolStrings> that are also available for checking due to the equivalent a "using" statement,
#                  or undef if none.
#       resolvingFlags - The <Resolving Flags> to use with this reference.  They are ignored if the type is <REFERENCE_TEXT>.
#
#   Returns:
#
#       The encoded <ReferenceString>.
#
sub MakeFrom #(type, symbol, scope, using, resolvingFlags)
    {
    my ($self, $type, $symbol, $scope, $using, $resolvingFlags) = @_;

    if ($type == ::REFERENCE_TEXT() || $resolvingFlags == 0)
       {  $resolvingFlags = undef;  };

    # The format is [type] 0x1E [resolving flags] \x1E [symbol] 0x1E [scope] ( 0x1E [using] )*
    # The format of the symbol, scope, and usings are the identifiers separated by 0x1F characters.
    # If scope is undef but using isn't, there will be two 0x1E's to signify the missing scope.

    my @identifiers = NaturalDocs::SymbolString->IdentifiersOf($symbol);

    my $string = $type . "\x1E" . $resolvingFlags . "\x1E" . join("\x1F", @identifiers);

    if (defined $scope)
        {
        @identifiers = NaturalDocs::SymbolString->IdentifiersOf($scope);
        $string .= "\x1E" . join("\x1F", @identifiers);
        };

    if (defined $using)
        {
        my @usingStrings;

        foreach my $using (@$using)
            {
            @identifiers = NaturalDocs::SymbolString->IdentifiersOf($using);
            push @usingStrings, join("\x1F", @identifiers);
            };

        if (!defined $scope)
            {  $string .= "\x1E";  };

        $string .= "\x1E" . join("\x1E", @usingStrings);
        };

    return $string;
    };


#
#   Function: ToBinaryFile
#
#   Writes a <ReferenceString> to the passed filehandle.  Can also encode an undef.
#
#   Parameters:
#
#       fileHandle - The filehandle to write to.
#       referenceString - The <ReferenceString> to write, or undef.
#       binaryFormatFlags - Any <Binary Format Flags> you want to use to influence encoding.
#
#   Format:
#
#       > [SymbolString: Symbol or undef for an undef reference]
#       > [SymbolString: Scope or undef for none]
#       >
#       > [SymbolString: Using or undef for none]
#       > [SymbolString: Using or undef for no more]
#       > ...
#       >
#       > [UInt8: Type unless BINARYREF_NOTYPE is set]
#       > [UInt8: Resolving Flags unless BINARYREF_NORESOLVINGFLAGS is set]
#
#   Dependencies:
#
#       - <ReferenceTypes> must fit into a UInt8.  All values must be <= 255.
#       - All <Resolving Flags> must fit into a UInt8.  All values must be <= 255.
#
sub ToBinaryFile #(fileHandle, referenceString, binaryFormatFlags)
    {
    my ($self, $fileHandle, $referenceString, $binaryFormatFlags) = @_;

    my ($type, $symbol, $scope, $using, $resolvingFlags) = $self->InformationOf($referenceString);

    # [SymbolString: Symbol or undef for an undef reference]

    NaturalDocs::SymbolString->ToBinaryFile($fileHandle, $symbol);

    # [SymbolString: scope or undef if none]

    NaturalDocs::SymbolString->ToBinaryFile($fileHandle, $scope);

    # [SymbolString: using or undef if none/no more] ...

    if (defined $using)
        {
        foreach my $usingScope (@$using)
            {  NaturalDocs::SymbolString->ToBinaryFile($fileHandle, $usingScope);  };
        };

    NaturalDocs::SymbolString->ToBinaryFile($fileHandle, undef);

    # [UInt8: Type unless BINARYREF_NOTYPE is set]

    if (!($binaryFormatFlags & BINARYREF_NOTYPE))
        {  print $fileHandle pack('C', $type);  };

    # [UInt8: Resolving Flags unless BINARYREF_NORESOLVINGFLAGS is set]

    if (!($binaryFormatFlags & BINARYREF_NORESOLVINGFLAGS))
        {  print $fileHandle pack('C', $type);  };
    };


#
#   Function: FromBinaryFile
#
#   Reads a <ReferenceString> or undef from the passed filehandle.
#
#   Parameters:
#
#       fileHandle - The filehandle to read from.
#       binaryFormatFlags - Any <Binary Format Flags> you want to use to influence decoding.
#       type - The <ReferenceType> to use if <BINARYREF_NOTYPE> is set.
#       resolvingFlags - The <Resolving Flags> to use if <BINARYREF_NORESOLVINGFLAGS> is set.
#
#   Returns:
#
#       The <ReferenceString> or undef.
#
#   See Also:
#
#       See <ToBinaryFile()> for format and dependencies.
#
sub FromBinaryFile #(fileHandle, binaryFormatFlags, type, resolvingFlags)
    {
    my ($self, $fileHandle, $binaryFormatFlags, $type, $resolvingFlags) = @_;

    # [SymbolString: Symbol or undef for an undef reference]

    my $symbol = NaturalDocs::SymbolString->FromBinaryFile($fileHandle);

    if (!defined $symbol)
        {  return undef;  };

    # [SymbolString: scope or undef if none]

    my $scope = NaturalDocs::SymbolString->FromBinaryFile($fileHandle);

    # [SymbolString: using or undef if none/no more] ...

    my $usingSymbol;
    my @using;

    while ($usingSymbol = NaturalDocs::SymbolString->FromBinaryFile($fileHandle))
        {  push @using, $usingSymbol;  };

    if (scalar @using)
        {  $usingSymbol = \@using;  }
    else
        {  $usingSymbol = undef;  };

    # [UInt8: Type unless BINARYREF_NOTYPE is set]

    if (!($binaryFormatFlags & BINARYREF_NOTYPE))
        {
        my $raw;
        read($fileHandle, $raw, 1);
        $type = unpack('C', $raw);
        };

    # [UInt8: Resolving Flags unless BINARYREF_NORESOLVINGFLAGS is set]

    if (!($binaryFormatFlags & BINARYREF_NORESOLVINGFLAGS))
        {
        my $raw;
        read($fileHandle, $raw, 1);
        $resolvingFlags = unpack('C', $raw);
        };

    return $self->MakeFrom($type, $symbol, $scope, $usingSymbol, $resolvingFlags);
    };


#
#   Function: InformationOf
#
#   Returns the information encoded in a <ReferenceString>.
#
#   Parameters:
#
#       referenceString - The <ReferenceString> to decode.
#
#   Returns:
#
#       The array ( type, symbol, scope, using, resolvingFlags ).
#
#       type - The <ReferenceType>.
#       symbol - The <SymbolString>.
#       scope - The scope <SymbolString>, or undef if none.
#       using - An arrayref of scope <SymbolStrings> that the reference also has access to via "using" statements, or undef if none.
#       resolvingFlags - The <Resolving Flags> of the reference.
#
sub InformationOf #(referenceString)
    {
    my ($self, $referenceString) = @_;

    my ($type, $resolvingFlags, $symbolString, $scopeString, @usingStrings) = split(/\x1E/, $referenceString);

    if (!length $resolvingFlags)
        {  $resolvingFlags = undef;  };

    my @identifiers = split(/\x1F/, $symbolString);
    my $symbol = NaturalDocs::SymbolString->Join(@identifiers);

    my $scope;
    if (defined $scopeString && length($scopeString))
        {
        @identifiers = split(/\x1F/, $scopeString);
        $scope = NaturalDocs::SymbolString->Join(@identifiers);
        };

    my $using;
    if (scalar @usingStrings)
        {
        $using = [ ];
        foreach my $usingString (@usingStrings)
            {
            @identifiers = split(/\x1F/, $usingString);
            push @$using, NaturalDocs::SymbolString->Join(@identifiers);
            };
        };

    return ( $type, $symbol, $scope, $using, $resolvingFlags );
    };


#
#   Function: TypeOf
#
#   Returns the <ReferenceType> encoded in the reference string.  This is faster than <InformationOf()> if this is
#   the only information you need.
#
sub TypeOf #(referenceString)
    {
    my ($self, $referenceString) = @_;

    $referenceString =~ /^([^\x1E]+)/;
    return $1;
    };


1;
