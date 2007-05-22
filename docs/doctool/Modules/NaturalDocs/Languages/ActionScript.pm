###############################################################################
#
#   Class: NaturalDocs::Languages::ActionScript
#
###############################################################################
#
#   A subclass to handle the language variations of Flash ActionScript.
#
#
#   Topic: Language Support
#
#       Supported:
#
#       Not supported yet:
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL

use strict;
use integer;

package NaturalDocs::Languages::ActionScript;

use base 'NaturalDocs::Languages::Advanced';


################################################################################
# Group: Package Variables

#
#   hash: classModifiers
#   An existence hash of all the acceptable class modifiers.  The keys are in all lowercase.
#
my %classModifiers = ( 'dynamic' => 1,
                                   'intrinsic' => 1 );

#
#   hash: memberModifiers
#   An existence hash of all the acceptable class member modifiers.  The keys are in all lowercase.
#
my %memberModifiers = ( 'public' => 1,
                                        'private' => 1,
                                        'static' => 1 );


#
#   hash: declarationEnders
#   An existence hash of all the tokens that can end a declaration.  This is important because statements don't require a semicolon
#   to end.  The keys are in all lowercase.
#
my %declarationEnders = ( ';' => 1,
                                        '}' => 1,
                                        '{' => 1,
                                        'public' => 1,
                                        'private' => 1,
                                        'static' => 1,
                                        'class' => 1,
                                        'interface' => 1,
                                        'var' => 1,
                                        'function' => 1,
                                        'import' => 1 );



################################################################################
# Group: Interface Functions


#
#   Function: PackageSeparator
#   Returns the package separator symbol.
#
sub PackageSeparator
    {  return '.';  };


#
#   Function: EnumValues
#   Returns the <EnumValuesType> that describes how the language handles enums.
#
sub EnumValues
    {  return ::ENUM_GLOBAL();  };


#
#   Function: ParseParameterLine
#   Parses a prototype parameter line and returns it as a <NaturalDocs::Languages::Prototype::Parameter> object.
#
sub ParseParameterLine #(line)
    {
    my ($self, $line) = @_;
    return $self->ParsePascalParameterLine($line);
    };


#
#   Function: TypeBeforeParameter
#   Returns whether the type appears before the parameter in prototypes.
#
sub TypeBeforeParameter
    {  return 0;  };


#
#   Function: ParseFile
#
#   Parses the passed source file, sending comments acceptable for documentation to <NaturalDocs::Parser->OnComment()>.
#
#   Parameters:
#
#       sourceFile - The <FileName> to parse.
#       topicList - A reference to the list of <NaturalDocs::Parser::ParsedTopics> being built by the file.
#
#   Returns:
#
#       The array ( autoTopics, scopeRecord ).
#
#       autoTopics - An arrayref of automatically generated topics from the file, or undef if none.
#       scopeRecord - An arrayref of <NaturalDocs::Languages::Advanced::ScopeChanges>, or undef if none.
#
sub ParseFile #(sourceFile, topicsList)
    {
    my ($self, $sourceFile, $topicsList) = @_;

    $self->ParseForCommentsAndTokens($sourceFile, [ '//' ], [ '/*', '*/' ] );

    my $tokens = $self->Tokens();
    my $index = 0;
    my $lineNumber = 1;

    while ($index < scalar @$tokens)
        {
        if ($self->TryToSkipWhitespace(\$index, \$lineNumber) ||
            $self->TryToGetImport(\$index, \$lineNumber) ||
            $self->TryToGetClass(\$index, \$lineNumber) ||
            $self->TryToGetFunction(\$index, \$lineNumber) ||
            $self->TryToGetVariable(\$index, \$lineNumber) )
            {
            # The functions above will handle everything.
            }

        elsif ($tokens->[$index] eq '{')
            {
            $self->StartScope('}', $lineNumber, undef, undef, undef);
            $index++;
            }

        elsif ($tokens->[$index] eq '}')
            {
            if ($self->ClosingScopeSymbol() eq '}')
                {  $self->EndScope($lineNumber);  };

            $index++;
            }

        else
            {
            $self->SkipToNextStatement(\$index, \$lineNumber);
            };
        };


    # Don't need to keep these around.
    $self->ClearTokens();


    my $autoTopics = $self->AutoTopics();

    my $scopeRecord = $self->ScopeRecord();
    if (defined $scopeRecord && !scalar @$scopeRecord)
        {  $scopeRecord = undef;  };

    return ( $autoTopics, $scopeRecord );
    };



################################################################################
# Group: Statement Parsing Functions
# All functions here assume that the current position is at the beginning of a statement.
#
# Note for developers: I am well aware that the code in these functions do not check if we're past the end of the tokens as
# often as it should.  We're making use of the fact that Perl will always return undef in these cases to keep the code simpler.


#
#   Function: TryToGetIdentifier
#
#   Determines whether the position is at an identifier, and if so, skips it and returns the complete identifier as a string.  Returns
#   undef otherwise.
#
#   Parameters:
#
#       indexRef - A reference to the current token index.
#       lineNumberRef - A reference to the current line number.
#       allowStar - If set, allows the last identifier to be a star.
#
sub TryToGetIdentifier #(indexRef, lineNumberRef, allowStar)
    {
    my ($self, $indexRef, $lineNumberRef, $allowStar) = @_;
    my $tokens = $self->Tokens();

    my $index = $$indexRef;

    use constant MODE_IDENTIFIER_START => 1;
    use constant MODE_IN_IDENTIFIER => 2;
    use constant MODE_AFTER_STAR => 3;

    my $identifier;
    my $mode = MODE_IDENTIFIER_START;

    while ($index < scalar @$tokens)
        {
        if ($mode == MODE_IDENTIFIER_START)
            {
            if ($tokens->[$index] =~ /^[a-z\$\_]/i)
                {
                $identifier .= $tokens->[$index];
                $index++;

                $mode = MODE_IN_IDENTIFIER;
                }
            elsif ($allowStar && $tokens->[$index] eq '*')
                {
                $identifier .= '*';
                $index++;

                $mode = MODE_AFTER_STAR;
                }
            else
                {  return undef;  };
            }

        elsif ($mode == MODE_IN_IDENTIFIER)
            {
            if ($tokens->[$index] eq '.')
                {
                $identifier .= '.';
                $index++;

                $mode = MODE_IDENTIFIER_START;
                }
            elsif ($tokens->[$index] =~ /^[a-z0-9\$\_]/i)
                {
                $identifier .= $tokens->[$index];
                $index++;
                }
            else
                {  last;  };
            }

        else #($mode == MODE_AFTER_STAR)
            {
            if ($tokens->[$index] =~ /^[a-z0-9\$\_\.]/i)
                {  return undef;  }
            else
                {  last;  };
            };
        };

    # We need to check again because we may have run out of tokens after a dot.
    if ($mode != MODE_IDENTIFIER_START)
        {
        $$indexRef = $index;
        return $identifier;
        }
    else
        {  return undef;  };
    };


#
#   Function: TryToGetImport
#
#   Determines whether the position is at a import statement, and if so, adds it as a Using statement to the current scope, skips
#   it, and returns true.
#
sub TryToGetImport #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    my $index = $$indexRef;
    my $lineNumber = $$lineNumberRef;

    if ($tokens->[$index] ne 'import')
        {  return undef;  };

    $index++;
    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    my $identifier = $self->TryToGetIdentifier(\$index, \$lineNumber, 1);
    if (!$identifier)
        {  return undef;  };


    # Currently we implement importing by stripping the last package level and treating it as a using.  So "import p1.p2.p3" makes
    # p1.p2 the using path, which is over-tolerant but that's okay.  "import p1.p2.*" is treated the same way, but in this case it's
    # not over-tolerant.  If there's no dot, there's no point to including it.

    if (index($identifier, '.') != -1)
        {
        $identifier =~ s/\.[^\.]+$//;
        $self->AddUsing( NaturalDocs::SymbolString->FromText($identifier) );
        };

    $$indexRef = $index;
    $$lineNumberRef = $lineNumber;

    return 1;
    };


#
#   Function: TryToGetClass
#
#   Determines whether the position is at a class declaration statement, and if so, generates a topic for it, skips it, and
#   returns true.
#
#   Supported Syntaxes:
#
#       - Classes
#       - Interfaces
#       - Classes and interfaces with _global
#
sub TryToGetClass #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    my $index = $$indexRef;
    my $lineNumber = $$lineNumberRef;

    my @modifiers;

    while ($tokens->[$index] =~ /^[a-z]/i &&
              exists $classModifiers{lc($tokens->[$index])} )
        {
        push @modifiers, lc($tokens->[$index]);
        $index++;

        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        };

    my $type;

    if ($tokens->[$index] eq 'class' || $tokens->[$index] eq 'interface')
        {
        $type = $tokens->[$index];

        $index++;
        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        }
    else
        {  return undef;  };

    my $className = $self->TryToGetIdentifier(\$index, \$lineNumber);

    if (!$className)
        {  return undef;  };

    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    my @parents;

    if ($tokens->[$index] eq 'extends')
        {
        $index++;
        $self->TryToSkipWhitespace(\$index, \$lineNumber);

        my $parent = $self->TryToGetIdentifier(\$index, \$lineNumber);
        if (!$parent)
            {  return undef;  };

        push @parents, $parent;

        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        };

    if ($type eq 'class' && $tokens->[$index] eq 'implements')
        {
        $index++;
        $self->TryToSkipWhitespace(\$index, \$lineNumber);

        for (;;)
            {
            my $parent = $self->TryToGetIdentifier(\$index, \$lineNumber);
            if (!$parent)
                {  return undef;  };

            push @parents, $parent;

            $self->TryToSkipWhitespace(\$index, \$lineNumber);

            if ($tokens->[$index] ne ',')
                {  last;  }
            else
                {
                $index++;
                $self->TryToSkipWhitespace(\$index, \$lineNumber);
                };
            };
        };

    if ($tokens->[$index] ne '{')
        {  return undef;  };

    $index++;


    # If we made it this far, we have a valid class declaration.

    my $topicType;

    if ($type eq 'interface')
        {  $topicType = ::TOPIC_INTERFACE();  }
    else
        {  $topicType = ::TOPIC_CLASS();  };

    $className =~ s/^_global.//;

    my $autoTopic = NaturalDocs::Parser::ParsedTopic->New($topicType, $className,
                                                                                         undef, $self->CurrentUsing(),
                                                                                         undef,
                                                                                         undef, undef, $$lineNumberRef);

    $self->AddAutoTopic($autoTopic);
    NaturalDocs::Parser->OnClass($autoTopic->Package());

    foreach my $parent (@parents)
        {
        NaturalDocs::Parser->OnClassParent($autoTopic->Package(), NaturalDocs::SymbolString->FromText($parent),
                                                               undef, $self->CurrentUsing(), ::RESOLVE_ABSOLUTE());
        };

    $self->StartScope('}', $lineNumber, $autoTopic->Package());

    $$indexRef = $index;
    $$lineNumberRef = $lineNumber;

    return 1;
    };


#
#   Function: TryToGetFunction
#
#   Determines if the position is on a function declaration, and if so, generates a topic for it, skips it, and returns true.
#
#   Supported Syntaxes:
#
#       - Functions
#       - Constructors
#       - Properties
#       - Functions with _global
#
sub TryToGetFunction #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    my $index = $$indexRef;
    my $lineNumber = $$lineNumberRef;

    my $startIndex = $index;
    my $startLine = $lineNumber;

    my @modifiers;

    while ($tokens->[$index] =~ /^[a-z]/i &&
              exists $memberModifiers{lc($tokens->[$index])} )
        {
        push @modifiers, lc($tokens->[$index]);
        $index++;

        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        };

    if ($tokens->[$index] ne 'function')
        {  return undef;  };
    $index++;

    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    my $type;

    if ($tokens->[$index] eq 'get' || $tokens->[$index] eq 'set')
        {
        # This can either be a property ("function get Something()") or a function name ("function get()").

        my $nextIndex = $index;
        my $nextLineNumber = $lineNumber;

        $nextIndex++;
        $self->TryToSkipWhitespace(\$nextIndex, \$nextLineNumber);

        if ($tokens->[$nextIndex] eq '(')
            {
            $type = ::TOPIC_FUNCTION();
            # Ignore the movement and let the code ahead pick it up as the name.
            }
        else
            {
            $type = ::TOPIC_PROPERTY();
            $index = $nextIndex;
            $lineNumber = $nextLineNumber;
            };
        }
    else
        {  $type = ::TOPIC_FUNCTION();  };

    my $name = $self->TryToGetIdentifier(\$index, \$lineNumber);
    if (!$name)
        {  return undef;  };

    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    if ($tokens->[$index] ne '(')
        {  return undef;  };

    $index++;
    $self->GenericSkipUntilAfter(\$index, \$lineNumber, ')');

    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    if ($tokens->[$index] eq ':')
        {
        $index++;

        $self->TryToSkipWhitespace(\$index, \$lineNumber);

        $self->TryToGetIdentifier(\$index, \$lineNumber);

        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        };


    my $prototype = $self->NormalizePrototype( $self->CreateString($startIndex, $index) );

    if ($tokens->[$index] eq '{')
        {  $self->GenericSkip(\$index, \$lineNumber);  }
    elsif (!exists $declarationEnders{$tokens->[$index]})
        {  return undef;  };


    my $scope = $self->CurrentScope();

    if ($name =~ s/^_global.//)
        {  $scope = undef;  };

    $self->AddAutoTopic(NaturalDocs::Parser::ParsedTopic->New($type, $name,
                                                                                              $scope, $self->CurrentUsing(),
                                                                                              $prototype,
                                                                                              undef, undef, $startLine));


    # We succeeded if we got this far.

    $$indexRef = $index;
    $$lineNumberRef = $lineNumber;

    return 1;
    };


#
#   Function: TryToGetVariable
#
#   Determines if the position is on a variable declaration statement, and if so, generates a topic for each variable, skips the
#   statement, and returns true.
#
#   Supported Syntaxes:
#
#       - Variables
#       - Variables with _global
#
sub TryToGetVariable #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    my $index = $$indexRef;
    my $lineNumber = $$lineNumberRef;

    my $startIndex = $index;
    my $startLine = $lineNumber;

    my @modifiers;

    while ($tokens->[$index] =~ /^[a-z]/i &&
              exists $memberModifiers{lc($tokens->[$index])} )
        {
        push @modifiers, lc($tokens->[$index]);
        $index++;

        $self->TryToSkipWhitespace(\$index, \$lineNumber);
        };

    if ($tokens->[$index] ne 'var')
        {  return undef;  };
    $index++;

    $self->TryToSkipWhitespace(\$index, \$lineNumber);

    my $endTypeIndex = $index;
    my @names;
    my @types;

    for (;;)
        {
        my $name = $self->TryToGetIdentifier(\$index, \$lineNumber);
        if (!$name)
            {  return undef;  };

        $self->TryToSkipWhitespace(\$index, \$lineNumber);

        my $type;

        if ($tokens->[$index] eq ':')
            {
            $index++;
            $self->TryToSkipWhitespace(\$index, \$lineNumber);

            $type = ': ' . $self->TryToGetIdentifier(\$index, \$lineNumber);

            $self->TryToSkipWhitespace(\$index, \$lineNumber);
            };

        if ($tokens->[$index] eq '=')
            {
            do
                {
                $self->GenericSkip(\$index, \$lineNumber);
                }
            while ($tokens->[$index] ne ',' && !exists $declarationEnders{$tokens->[$index]} && $index < scalar @$tokens);
            };

        push @names, $name;
        push @types, $type;

        if ($tokens->[$index] eq ',')
            {
            $index++;
            $self->TryToSkipWhitespace(\$index, \$lineNumber);
            }
        elsif (exists $declarationEnders{$tokens->[$index]})
            {  last;  }
        else
            {  return undef;  };
        };


    # We succeeded if we got this far.

    my $prototypePrefix = $self->CreateString($startIndex, $endTypeIndex);

    for (my $i = 0; $i < scalar @names; $i++)
        {
        my $prototype = $self->NormalizePrototype( $prototypePrefix . ' ' . $names[$i] . $types[$i]);
        my $scope = $self->CurrentScope();

        if ($names[$i] =~ s/^_global.//)
            {  $scope = undef;  };

        $self->AddAutoTopic(NaturalDocs::Parser::ParsedTopic->New(::TOPIC_VARIABLE(), $names[$i],
                                                                                                  $scope, $self->CurrentUsing(),
                                                                                                  $prototype,
                                                                                                  undef, undef, $startLine));
        };

    $$indexRef = $index;
    $$lineNumberRef = $lineNumber;

    return 1;
    };



################################################################################
# Group: Low Level Parsing Functions


#
#   Function: GenericSkip
#
#   Advances the position one place through general code.
#
#   - If the position is on a string, it will skip it completely.
#   - If the position is on an opening symbol, it will skip until the past the closing symbol.
#   - If the position is on whitespace (including comments), it will skip it completely.
#   - Otherwise it skips one token.
#
#   Parameters:
#
#       indexRef - A reference to the current index.
#       lineNumberRef - A reference to the current line number.
#
sub GenericSkip #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    # We can ignore the scope stack because we're just skipping everything without parsing, and we need recursion anyway.
    if ($tokens->[$$indexRef] eq '{')
        {
        $$indexRef++;
        $self->GenericSkipUntilAfter($indexRef, $lineNumberRef, '}');
        }
    elsif ($tokens->[$$indexRef] eq '(')
        {
        $$indexRef++;
        $self->GenericSkipUntilAfter($indexRef, $lineNumberRef, ')');
        }
    elsif ($tokens->[$$indexRef] eq '[')
        {
        $$indexRef++;
        $self->GenericSkipUntilAfter($indexRef, $lineNumberRef, ']');
        }

    elsif ($self->TryToSkipWhitespace($indexRef, $lineNumberRef) ||
            $self->TryToSkipString($indexRef, $lineNumberRef))
        {
        }

    else
        {  $$indexRef++;  };
    };


#
#   Function: GenericSkipUntilAfter
#
#   Advances the position via <GenericSkip()> until a specific token is reached and passed.
#
sub GenericSkipUntilAfter #(indexRef, lineNumberRef, token)
    {
    my ($self, $indexRef, $lineNumberRef, $token) = @_;
    my $tokens = $self->Tokens();

    while ($$indexRef < scalar @$tokens && $tokens->[$$indexRef] ne $token)
        {  $self->GenericSkip($indexRef, $lineNumberRef);  };

    if ($tokens->[$$indexRef] eq "\n")
        {  $$lineNumberRef++;  };
    $$indexRef++;
    };


#
#   Function: SkipToNextStatement
#
#   Advances the position via <GenericSkip()> until the next statement, which is defined as anything in <declarationEnders> not
#   appearing in brackets or strings.  It will always advance at least one token.
#
sub SkipToNextStatement #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    do
        {
        $self->GenericSkip($indexRef, $lineNumberRef);
        }
    while ( $$indexRef < scalar @$tokens &&
              !exists $declarationEnders{$tokens->[$$indexRef]} );
    };


#
#   Function: TryToSkipString
#   If the current position is on a string delimiter, skip past the string and return true.
#
#   Parameters:
#
#       indexRef - A reference to the index of the position to start at.
#       lineNumberRef - A reference to the line number of the position.
#
#   Returns:
#
#       Whether the position was at a string.
#
#   Syntax Support:
#
#       - Supports quotes, apostrophes, and at-quotes.
#
sub TryToSkipString #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;

    return ($self->SUPER::TryToSkipString($indexRef, $lineNumberRef, '\'') ||
               $self->SUPER::TryToSkipString($indexRef, $lineNumberRef, '"') );
    };


#
#   Function: TryToSkipWhitespace
#   If the current position is on a whitespace token, a line break token, or a comment, it skips them and returns true.  If there are
#   a number of these in a row, it skips them all.
#
sub TryToSkipWhitespace #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    my $result;

    while ($$indexRef < scalar @$tokens)
        {
        if ($tokens->[$$indexRef] =~ /^[ \t]/)
            {
            $$indexRef++;
            $result = 1;
            }
        elsif ($tokens->[$$indexRef] eq "\n")
            {
            $$indexRef++;
            $$lineNumberRef++;
            $result = 1;
            }
        elsif ($self->TryToSkipComment($indexRef, $lineNumberRef))
            {
            $result = 1;
            }
        else
            {  last;  };
        };

    return $result;
    };


#
#   Function: TryToSkipComment
#   If the current position is on a comment, skip past it and return true.
#
sub TryToSkipComment #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;

    return ( $self->TryToSkipLineComment($indexRef, $lineNumberRef) ||
                $self->TryToSkipMultilineComment($indexRef, $lineNumberRef) );
    };


#
#   Function: TryToSkipLineComment
#   If the current position is on a line comment symbol, skip past it and return true.
#
sub TryToSkipLineComment #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    if ($tokens->[$$indexRef] eq '/' && $tokens->[$$indexRef+1] eq '/')
        {
        $self->SkipRestOfLine($indexRef, $lineNumberRef);
        return 1;
        }
    else
        {  return undef;  };
    };


#
#   Function: TryToSkipMultilineComment
#   If the current position is on an opening comment symbol, skip past it and return true.
#
sub TryToSkipMultilineComment #(indexRef, lineNumberRef)
    {
    my ($self, $indexRef, $lineNumberRef) = @_;
    my $tokens = $self->Tokens();

    if ($tokens->[$$indexRef] eq '/' && $tokens->[$$indexRef+1] eq '*')
        {
        $self->SkipUntilAfter($indexRef, $lineNumberRef, '*', '/');
        return 1;
        }
    else
        {  return undef;  };
    };


1;
