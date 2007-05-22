###############################################################################
#
#   Package: NaturalDocs::Builder::HTMLBase
#
###############################################################################
#
#   A base package for all the shared functionality in <NaturalDocs::Builder::HTML> and
#   <NaturalDocs::Builder::FramedHTML>.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL


use Tie::RefHash;

use strict;
use integer;

package NaturalDocs::Builder::HTMLBase;

use base 'NaturalDocs::Builder::Base';


###############################################################################
# Group: Package Variables
# These variables are shared by all instances of the package so don't change them.


#
#   handle: FH_CSS_FILE
#
#   The file handle to use when updating CSS files.
#


#
#   Hash: abbreviations
#
#   An existence hash of acceptable abbreviations.  These are words that <AddDoubleSpaces()> won't put a second space
#   after when followed by period-whitespace-capital letter.  Yes, this is seriously over-engineered.
#
my %abbreviations = ( mr => 1, mrs => 1, ms => 1, dr => 1,
                                  rev => 1, fr => 1, 'i.e' => 1,
                                  maj => 1, gen => 1, pres => 1, sen => 1, rep => 1,
                                  n => 1, s => 1, e => 1, w => 1, ne => 1, se => 1, nw => 1, sw => 1 );

#
#   array: indexHeadings
#
#   An array of the headings of all the index sections.  First is for symbols, second for numbers, and the rest for each letter.
#
my @indexHeadings = ( '$#!', '0-9', 'A' .. 'Z' );

#
#   array: indexAnchors
#
#   An array of the HTML anchors of all the index sections.  First is for symbols, second for numbers, and the rest for each letter.
#
my @indexAnchors = ( 'Symbols', 'Numbers', 'A' .. 'Z' );

#
#   bool: saidUpdatingCSSFile
#
#   Whether the status message "Updating CSS file..." has been displayed.  We only want to print it once, no matter how many
#   HTML-based targets we are building.
#
my $saidUpdatingCSSFile;

#
#   constant: ADD_HIDDEN_BREAKS
#
#   Just a synonym for "1" so that setting the flag on <StringToHTML()> is clearer in the calling code.
#
use constant ADD_HIDDEN_BREAKS => 1;


###############################################################################
# Group: ToolTip Package Variables
#
#   These variables are for the tooltip generation functions only.  Since they're reset on every call to <BuildContent()> and
#   <BuildIndexPages()>, and are only used by them and their support functions, they can be shared by all instances of the
#   package.

#
#   int: tooltipLinkNumber
#
#   A number used as part of the ID for each link that has a tooltip.  Should be incremented whenever one is made.
#
my $tooltipLinkNumber;

#
#   int: tooltipNumber
#
#   A number used as part of the ID for each tooltip.  Should be incremented whenever one is made.
#
my $tooltipNumber;

#
#   hash: tooltipSymbolsToNumbers
#
#   A hash that maps the tooltip symbols to their assigned numbers.
#
my %tooltipSymbolsToNumbers;

#
#   string: tooltipHTML
#
#   The generated tooltip HTML.
#
my $tooltipHTML;


###############################################################################
# Group: Menu Package Variables
#
#   These variables are for the menu generation functions only.  Since they're reset on every call to <BuildMenu()> and are
#   only used by it and its support functions, they can be shared by all instances of the package.
#


#
#   hash: prebuiltMenus
#
#   A hash that maps output directonies to menu HTML already built for it.  There will be no selection or JavaScript in the menus.
#
my %prebuiltMenus;


#
#   bool: menuNumbersAndLengthsDone
#
#   Set when the variables that only need to be calculated for the menu once are done.  This includes <menuGroupNumber>,
#   <menuLength>, <menuGroupLengths>, and <menuGroupNumbers>, and <menuRootLength>.
#
my $menuNumbersAndLengthsDone;


#
#   int: menuGroupNumber
#
#   The current menu group number.  Each time a group is created, this is incremented so that each one will be unique.
#
my $menuGroupNumber;


#
#   int: menuLength
#
#   The length of the entire menu, fully expanded.  The value is computed from the <Menu Length Constants>.
#
my $menuLength;


#
#   hash: menuGroupLengths
#
#   A hash of the length of each group, *not* including any subgroup contents.  The keys are references to each groups'
#   <NaturalDocs::Menu::Entry> object, and the values are their lengths computed from the <Menu Length Constants>.
#
my %menuGroupLengths;
tie %menuGroupLengths, 'Tie::RefHash';


#
#   hash: menuGroupNumbers
#
#   A hash of the number of each group, as managed by <menuGroupNumber>.  The keys are references to each groups'
#   <NaturalDocs::Menu::Entry> object, and the values are the number.
#
my %menuGroupNumbers;
tie %menuGroupNumbers, 'Tie::RefHash';


#
#   int: menuRootLength
#
#   The length of the top-level menu entries without expansion.  The value is computed from the <Menu Length Constants>.
#
my $menuRootLength;


#
#   constants: Menu Length Constants
#
#   Constants used to approximate the lengths of the menu or its groups.
#
#   MENU_TITLE_LENGTH       - The length of the title.
#   MENU_SUBTITLE_LENGTH - The length of the subtitle.
#   MENU_FILE_LENGTH         - The length of one file entry.
#   MENU_GROUP_LENGTH     - The length of one group entry.
#   MENU_TEXT_LENGTH        - The length of one text entry.
#   MENU_LINK_LENGTH        - The length of one link entry.
#
#   MENU_LENGTH_LIMIT    - The limit of the menu's length.  If the total length surpasses this limit, groups that aren't required
#                                       to be open to show the selection will default to closed on browsers that support it.
#
use constant MENU_TITLE_LENGTH => 3;
use constant MENU_SUBTITLE_LENGTH => 1;
use constant MENU_FILE_LENGTH => 1;
use constant MENU_GROUP_LENGTH => 2; # because it's a line and a blank space
use constant MENU_TEXT_LENGTH => 1;
use constant MENU_LINK_LENGTH => 1;
use constant MENU_INDEX_LENGTH => 1;

use constant MENU_LENGTH_LIMIT => 35;


###############################################################################
# Group: Implemented Interface Functions
#
#   The behavior of these functions is shared between HTML output formats.
#


#
#   Function: PurgeFiles
#
#   Deletes the output files associated with the purged source files.
#
sub PurgeFiles
    {
    my $self = shift;

    my $filesToPurge = NaturalDocs::Project->FilesToPurge();

    # Combine directories into a hash to remove duplicate work.
    my %directoriesToPurge;

    foreach my $file (keys %$filesToPurge)
        {
        # It's possible that there may be files there that aren't in a valid input directory anymore.  They won't generate an output
        # file name so we need to check for undef.
        my $outputFile = $self->OutputFileOf($file);
        if (defined $outputFile)
            {
            unlink($outputFile);
            $directoriesToPurge{ NaturalDocs::File->NoFileName($outputFile) } = 1;
            };
        };

    foreach my $directory (keys %directoriesToPurge)
        {
        NaturalDocs::File->RemoveEmptyTree($directory, NaturalDocs::Settings->OutputDirectoryOf($self));
        };
    };


#
#   Function: PurgeIndexes
#
#   Deletes the output files associated with the purged source files.
#
#   Parameters:
#
#       indexes  - An existence hashref of the index types to purge.  The keys are the <TopicTypes> or * for the general index.
#
sub PurgeIndexes #(indexes)
    {
    my ($self, $indexes) = @_;

    foreach my $index (keys %$indexes)
        {
        $self->PurgeIndexFiles($index, undef);
        };
    };


#
#   Function: BeginBuild
#
#   Creates the necessary subdirectories in the output directory.
#
sub BeginBuild #(hasChanged)
    {
    my ($self, $hasChanged) = @_;

    foreach my $directory ( $self->JavaScriptDirectory(), $self->CSSDirectory(), $self->IndexDirectory() )
        {
        if (!-d $directory)
            {  NaturalDocs::File->CreatePath($directory);  };
        };
    };


#
#   Function: EndBuild
#
#   Synchronizes the projects CSS and JavaScript files.
#
sub EndBuild #(hasChanged)
    {
    my ($self, $hasChanged) = @_;


    # Update the style sheets.

    my $styles = NaturalDocs::Settings->Styles();
    my $changed;

    my $cssDirectory = $self->CSSDirectory();
    my $mainCSSFile = $self->MainCSSFile();

    for (my $i = 0; $i < scalar @$styles; $i++)
        {
        my $outputCSSFile;

        if (scalar @$styles == 1)
            {  $outputCSSFile = $mainCSSFile;  }
        else
            {  $outputCSSFile = NaturalDocs::File->JoinPaths($cssDirectory, ($i + 1) . '.css');  };


        my $masterCSSFile = NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), $styles->[$i] . '.css' );

        if (! -e $masterCSSFile)
            {  $masterCSSFile = NaturalDocs::File->JoinPaths( NaturalDocs::Settings->StyleDirectory(), $styles->[$i] . '.css' );  };

        # We check both the date and the size in case the user switches between two styles which just happen to have the same
        # date.  Should rarely happen, but it might.
        if (! -e $outputCSSFile ||
            (stat($masterCSSFile))[9] != (stat($outputCSSFile))[9] ||
             -s $masterCSSFile != -s $outputCSSFile)
            {
            if (!NaturalDocs::Settings->IsQuiet() && !$saidUpdatingCSSFile)
                {
                print "Updating CSS file...\n";
                $saidUpdatingCSSFile = 1;
                };

            NaturalDocs::File->Copy($masterCSSFile, $outputCSSFile);

            $changed = 1;
            };
        };


    my $deleteFrom;

    if (scalar @$styles == 1)
        {  $deleteFrom = 1;  }
    else
        {  $deleteFrom = scalar @$styles + 1;  };

    for (;;)
        {
        my $file = NaturalDocs::File->JoinPaths($cssDirectory, $deleteFrom . '.css');

        if (! -e $file)
            {  last;  };

        unlink ($file);
        $deleteFrom++;

        $changed = 1;
        };


    if ($changed)
        {
        if (scalar @$styles > 1)
            {
            open(FH_CSS_FILE, '>' . $mainCSSFile);

            for (my $i = 0; $i < scalar @$styles; $i++)
                {
                print FH_CSS_FILE '@import URL("' . ($i + 1) . '.css");' . "\n";
                };

            close(FH_CSS_FILE);
            };
        };



    # Update the JavaScript file.

    my $jsMaster = NaturalDocs::File->JoinPaths( NaturalDocs::Settings->JavaScriptDirectory(), 'NaturalDocs.js' );
    my $jsOutput = $self->MainJavaScriptFile();

    # We check both the date and the size in case the user switches between two styles which just happen to have the same
    # date.  Should rarely happen, but it might.
    if (! -e $jsOutput ||
        (stat($jsMaster))[9] != (stat($jsOutput))[9] ||
         -s $jsMaster != -s $jsOutput)
        {
        NaturalDocs::File->Copy($jsMaster, $jsOutput);
        };
    };



###############################################################################
# Group: Section Functions


#
#   function: BuildTitle
#
#   Builds and returns the HTML page title of a file.
#
#   Parameters:
#
#       sourceFile - The source <FileName> to build the title of.
#
#   Returns:
#
#       The source file's title in HTML.
#
sub BuildTitle #(sourceFile)
    {
    my ($self, $sourceFile) = @_;

    # If we have a menu title, the page title is [menu title] - [file title].  Otherwise it is just [file title].

    my $title = NaturalDocs::Project->DefaultMenuTitleOf($sourceFile);

    my $menuTitle = NaturalDocs::Menu->Title();
    if (defined $menuTitle && $menuTitle ne $title)
        {  $title .= ' - ' . $menuTitle;  };

    $title = $self->StringToHTML($title);

    return $title;
    };

#
#   function: BuildMenu
#
#   Builds and returns the side menu of a file.
#
#   Parameters:
#
#       sourceFile - The source <FileName> to use if you're looking for a source file.
#       indexType - The index <TopicType> to use if you're looking for an index.
#       isFramed - Whether the menu will appear in a frame.  If so, it assumes the <base> HTML tag is set to make links go to the
#                       appropriate frame.
#
#       Both sourceFile and indexType may be undef.
#
#   Returns:
#
#       The side menu in HTML.
#
sub BuildMenu #(FileName sourceFile, TopicType indexType, bool isFramed) -> string htmlMenu
    {
    my ($self, $sourceFile, $indexType, $isFramed) = @_;

    if (!$menuNumbersAndLengthsDone)
        {
        $menuGroupNumber = 1;
        $menuLength = 0;
        %menuGroupLengths = ( );
        %menuGroupNumbers = ( );
        $menuRootLength = 0;
        };

    my $outputDirectory;

    if ($sourceFile)
        {  $outputDirectory = NaturalDocs::File->NoFileName( $self->OutputFileOf($sourceFile) );  }
    elsif ($indexType)
        {  $outputDirectory = NaturalDocs::File->NoFileName( $self->IndexFileOf($indexType) );  }
    else
        {  $outputDirectory = NaturalDocs::Settings->OutputDirectoryOf($self);  };


    # Comment needed for UpdateFile().
    my $output = '<!--START_ND_MENU-->';


    if (!exists $prebuiltMenus{$outputDirectory})
        {
        my $segmentOutput;

        ($segmentOutput, $menuRootLength) =
            $self->BuildMenuSegment($outputDirectory, $isFramed, NaturalDocs::Menu->Content());

        my $titleOutput;

        my $menuTitle = NaturalDocs::Menu->Title();
        if (defined $menuTitle)
            {
            if (!$menuNumbersAndLengthsDone)
                {  $menuLength += MENU_TITLE_LENGTH;  };

            $menuRootLength += MENU_TITLE_LENGTH;

            $titleOutput .=
            '<div class=MTitle>'
                . $self->StringToHTML($menuTitle);

            my $menuSubTitle = NaturalDocs::Menu->SubTitle();
            if (defined $menuSubTitle)
                {
                if (!$menuNumbersAndLengthsDone)
                    {  $menuLength += MENU_SUBTITLE_LENGTH;  };

                $menuRootLength += MENU_SUBTITLE_LENGTH;

                $titleOutput .=
                '<div class=MSubTitle>'
                    . $self->StringToHTML($menuSubTitle)
                . '</div>';
                };

            $titleOutput .=
            '</div>';
            };

        $prebuiltMenus{$outputDirectory} = $titleOutput . $segmentOutput;
        $output .= $titleOutput . $segmentOutput;
        }
    else
        {  $output .= $prebuiltMenus{$outputDirectory};  };


    # Highlight the menu selection.

    if ($sourceFile)
        {
        # Dependency: This depends on how BuildMenuSegment() formats file entries.
        my $outputFile = $self->OutputFileOf($sourceFile);
        my $tag = '<div class=MFile><a href="' . $self->MakeRelativeURL($outputDirectory, $outputFile) . '">';
        my $tagIndex = index($output, $tag);

        if ($tagIndex != -1)
            {
            my $endIndex = index($output, '</a>', $tagIndex);

            substr($output, $endIndex, 4, '');
            substr($output, $tagIndex, length($tag), '<div class=MFile id=MSelected>');
            };
        }
    elsif ($indexType)
        {
        # Dependency: This depends on how BuildMenuSegment() formats index entries.
        my $outputFile = $self->IndexFileOf($indexType);
        my $tag = '<div class=MIndex><a href="' . $self->MakeRelativeURL($outputDirectory, $outputFile) . '">';
        my $tagIndex = index($output, $tag);

        if ($tagIndex != -1)
            {
            my $endIndex = index($output, '</a>', $tagIndex);

            substr($output, $endIndex, 4, '');
            substr($output, $tagIndex, length($tag), '<div class=MIndex id=MSelected>');
            };
        };


    # If the completely expanded menu is too long, collapse all the groups that aren't in the selection hierarchy or near the
    # selection.  By doing this instead of having them default to closed via CSS, any browser that doesn't support changing this at
    # runtime will keep the menu entirely open so that its still usable.

    if ($menuLength > MENU_LENGTH_LIMIT())
        {
        my $menuSelectionHierarchy = $self->GetMenuSelectionHierarchy($sourceFile, $indexType);

        my $toExpand = $self->ExpandMenu($sourceFile, $indexType, $menuSelectionHierarchy, $menuRootLength);

        $output .=

        '<script language=JavaScript><!--' . "\n"

        # Using ToggleMenu here causes IE to sometimes say display is nothing instead of "block" or "none" on the first click.
        # Whatever.  This is just as good.

        . 'if (document.getElementById)'
            . '{';

            if (scalar @$toExpand)
                {
                $output .=

                'for (var menu = 1; menu < ' . $menuGroupNumber . '; menu++)'
                    . '{'
                    . 'if (menu != ' . join(' && menu != ', @$toExpand) . ')'
                        . '{'
                        . 'document.getElementById("MGroupContent" + menu).style.display = "none";'
                        . '};'
                    . '};'
                }
            else
                {
                $output .=

                'for (var menu = 1; menu < ' . $menuGroupNumber . '; menu++)'
                    . '{'
                    . 'document.getElementById("MGroupContent" + menu).style.display = "none";'
                    . '};'
                };

            $output .=
            '}'

        . '// --></script>';
        };

    # Comment needed for UpdateFile().
    $output .= '<!--END_ND_MENU-->';

    $menuNumbersAndLengthsDone = 1;

    return $output;
    };


#
#   Function: BuildMenuSegment
#
#   A recursive function to build a segment of the menu.  *Remember to reset the <Menu Package Variables> before calling this
#   for the first time.*
#
#   Parameters:
#
#       outputDirectory - The output directory the menu is being built for.
#       isFramed - Whether the menu will be in a HTML frame or not.  Assumes that if it is, the <base> HTML tag will be set so that
#                       links are directed to the proper frame.
#       menuSegment - An arrayref specifying the segment of the menu to build.  Either pass the menu itself or the contents
#                               of a group.
#
#   Returns:
#
#       The array ( menuHTML, length ).
#
#       menuHTML - The menu segment in HTML.
#       groupLength - The length of the group, *not* including the contents of any subgroups, as computed from the
#                            <Menu Length Constants>.
#
sub BuildMenuSegment #(outputDirectory, isFramed, menuSegment)
    {
    my ($self, $outputDirectory, $isFramed, $menuSegment) = @_;

    my ($output, $groupLength);

    foreach my $entry (@$menuSegment)
        {
        if ($entry->Type() == ::MENU_GROUP())
            {
            my ($entryOutput, $entryLength) =
                $self->BuildMenuSegment($outputDirectory, $isFramed, $entry->GroupContent());

            my $entryNumber;

            if (!$menuNumbersAndLengthsDone)
                {
                $entryNumber = $menuGroupNumber;
                $menuGroupNumber++;

                $menuGroupLengths{$entry} = $entryLength;
                $menuGroupNumbers{$entry} = $entryNumber;
                }
            else
                {  $entryNumber = $menuGroupNumbers{$entry};  };

            $output .=
            '<div class=MEntry>'
                . '<div class=MGroup>'

                    . '<a href="javascript:ToggleMenu(\'MGroupContent' . $entryNumber . '\')"'
                         . ($isFramed ? ' target="_self"' : '') . '>'
                        . $self->StringToHTML($entry->Title())
                    . '</a>'

                    . '<div class=MGroupContent id=MGroupContent' . $entryNumber . '>'
                        . $entryOutput
                    . '</div>'

                . '</div>'
            . '</div>';

            $groupLength += MENU_GROUP_LENGTH;
            }

        elsif ($entry->Type() == ::MENU_FILE())
            {
            my $targetOutputFile = $self->OutputFileOf($entry->Target());

        # Dependency: BuildMenu() depends on how this formats file entries.
            $output .=
            '<div class=MEntry>'
                . '<div class=MFile>'
                    . '<a href="' . $self->MakeRelativeURL($outputDirectory, $targetOutputFile) . '">'
                        . $self->StringToHTML( $entry->Title(), ADD_HIDDEN_BREAKS)
                    . '</a>'
                . '</div>'
            . '</div>';

            $groupLength += MENU_FILE_LENGTH;
            }

        elsif ($entry->Type() == ::MENU_TEXT())
            {
            $output .=
            '<div class=MEntry>'
                . '<div class=MText>'
                    . $self->StringToHTML( $entry->Title() )
                . '</div>'
            . '</div>';

            $groupLength += MENU_TEXT_LENGTH;
            }

        elsif ($entry->Type() == ::MENU_LINK())
            {
            $output .=
            '<div class=MEntry>'
                . '<div class=MLink>'
                    . '<a href="' . $entry->Target() . '"' . ($isFramed ? ' target="_top"' : '') . '>'
                        . $self->StringToHTML( $entry->Title() )
                    . '</a>'
                . '</div>'
            . '</div>';

            $groupLength += MENU_LINK_LENGTH;
            }

        elsif ($entry->Type() == ::MENU_INDEX())
            {
            my $indexFile = $self->IndexFileOf($entry->Target);

        # Dependency: BuildMenu() depends on how this formats index entries.
            $output .=
            '<div class=MEntry>'
                . '<div class=MIndex>'
                    . '<a href="' . $self->MakeRelativeURL( $outputDirectory, $self->IndexFileOf($entry->Target()) ) . '">'
                        . $self->StringToHTML( $entry->Title() )
                    . '</a>'
                . '</div>'
            . '</div>';

            $groupLength += MENU_INDEX_LENGTH;
            };
        };


    if (!$menuNumbersAndLengthsDone)
        {  $menuLength += $groupLength;  };

    return ($output, $groupLength);
    };


#
#   Function: BuildContent
#
#   Builds and returns the main page content.
#
#   Parameters:
#
#       sourceFile - The source <FileName>.
#       parsedFile - The parsed source file as an arrayref of <NaturalDocs::Parser::ParsedTopic> objects.
#
#   Returns:
#
#       The page content in HTML.
#
sub BuildContent #(sourceFile, parsedFile)
    {
    my ($self, $sourceFile, $parsedFile) = @_;

    $self->ResetToolTips();

    my $output;
    my $i = 0;

    while ($i < scalar @$parsedFile)
        {
        my $anchor = $self->SymbolToHTMLSymbol($parsedFile->[$i]->Symbol());

        my $scope = NaturalDocs::Topics->TypeInfo($parsedFile->[$i]->Type())->Scope();


        # The anchors are closed, but not around the text, so the :hover CSS style won't accidentally kick in.

        my $headerType;

        if ($i == 0)
            {  $headerType = 'h1';  }
        elsif ($scope == ::SCOPE_START() || $scope == ::SCOPE_END())
            {  $headerType = 'h2';  }
        else
            {  $headerType = 'h3';  };

        $output .=

        '<div class=C' . NaturalDocs::Topics->NameOfType($parsedFile->[$i]->Type(), 0, 1)
            . ($i == 0 ? ' id=MainTopic' : '') . '>'

            . '<div class=CTopic>'

            . '<' . $headerType . ' class=CTitle>'
                . '<a name="' . $anchor . '"></a>'
                . $self->StringToHTML( $parsedFile->[$i]->Title(), ADD_HIDDEN_BREAKS)
            . '</' . $headerType . '>';


        my $hierarchy;
        if (NaturalDocs::Topics->TypeInfo( $parsedFile->[$i]->Type() )->ClassHierarchy())
            {
            $hierarchy = $self->BuildClassHierarchy($sourceFile, $parsedFile->[$i]->Symbol());
            };

        my $summary;
        if ($i == 0 || $scope == ::SCOPE_START() || $scope == ::SCOPE_END())
            {
            $summary .= $self->BuildSummary($sourceFile, $parsedFile, $i);
            };

        my $hasBody;
        if (defined $hierarchy || defined $summary ||
            defined $parsedFile->[$i]->Prototype() || defined $parsedFile->[$i]->Body())
            {
            $output .= '<div class=CBody>';
            $hasBody = 1;
            };

        $output .= $hierarchy;

        if (defined $parsedFile->[$i]->Prototype())
            {
            $output .= $self->BuildPrototype($parsedFile->[$i]->Type(), $parsedFile->[$i]->Prototype(), $sourceFile);
            };

        if (defined $parsedFile->[$i]->Body())
            {
            $output .= $self->NDMarkupToHTML( $sourceFile, $parsedFile->[$i]->Body(), $parsedFile->[$i]->Symbol(),
                                                                  $parsedFile->[$i]->Package(), $parsedFile->[$i]->Type(),
                                                                  $parsedFile->[$i]->Using() );
            };

        $output .= $summary;


        if ($hasBody)
            {  $output .= '</div>';  };

        $output .=
            '</div>' # CTopic
        . '</div>' # CType
        . "\n\n";

        $i++;
        };

    return $output;
    };


#
#   Function: BuildSummary
#
#   Builds a summary, either for the entire file or the current class/section.
#
#   Parameters:
#
#       sourceFile - The source <FileName> the summary appears in.
#
#       parsedFile - A reference to the parsed source file.
#
#       index   - The index into the parsed file to start at.  If undef or zero, it builds a summary for the entire file.  If it's the
#                    index of a <TopicType> that starts or ends a scope, it builds a summary for that scope
#
#   Returns:
#
#       The summary in HTML.
#
sub BuildSummary #(sourceFile, parsedFile, index)
    {
    my ($self, $sourceFile, $parsedFile, $index) = @_;
    my $completeSummary;

    if (!defined $index || $index == 0)
        {
        $index = 0;
        $completeSummary = 1;
        }
    else
        {
        # Skip the scope entry.
        $index++;
        };

    if ($index + 1 >= scalar @$parsedFile)
        {  return undef;  };


    my $scope = NaturalDocs::Topics->TypeInfo($parsedFile->[$index]->Type())->Scope();

    # Return nothing if there's only one entry.
    if (!$completeSummary && ($scope == ::SCOPE_START() || $scope == ::SCOPE_END()) )
        {  return undef;  };


    my $indent = 0;
    my $inGroup;

    # In a nice efficiency twist, these buggers will hold the opening div tags if true, undef if false.  Not that this script is known
    # for its efficiency.  Not that Perl is known for its efficiency.  Anyway...
    my $isMarkedAttr;
    my $entrySizeAttr = ' class=SEntrySize';
    my $descriptionSizeAttr = ' class=SDescriptionSize';

    my $output =
    '<!--START_ND_SUMMARY-->'
    . '<div class=Summary><div class=STitle>Summary</div>'

        # Not all browsers get table padding right, so we need a div to apply the border.
        . '<div class=SBorder>'
        . '<table border=0 cellspacing=0 cellpadding=0 class=STable>';

        while ($index < scalar @$parsedFile)
            {
            my $topic = $parsedFile->[$index];
            my $scope = NaturalDocs::Topics->TypeInfo($topic->Type())->Scope();

            if (!$completeSummary && ($scope == ::SCOPE_START() || $scope == ::SCOPE_END()) )
                {  last;  };


            # Remove modifiers as appropriate for the current entry.

            if ($scope == ::SCOPE_START() || $scope == ::SCOPE_END())
                {
                $indent = 0;
                $inGroup = 0;
                $isMarkedAttr = undef;
                }
            elsif ($topic->Type() eq ::TOPIC_GROUP())
                {
                if ($inGroup)
                    {  $indent--;  };

                $inGroup = 0;
                $isMarkedAttr = undef;
                };


            $output .=
             '<tr' . $isMarkedAttr . '><td' . $entrySizeAttr . '>'
                . '<div class=S' . ($index == 0 ? 'Main' : NaturalDocs::Topics->NameOfType($topic->Type(), 0, 1)) . '>'
                    . '<div class=SEntry>';


            # Add any remaining modifiers to the HTML in the form of div tags.  This modifier approach isn't the most elegant
            # thing, but there's not a lot of options.  It works.

            if ($indent)
                {  $output .= '<div class=SIndent' . $indent . '>';  };


            # Add the entry itself.

            my $toolTipProperties;

            # We only want a tooltip here if there's a protoype.  Otherwise it's redundant.

            if (defined $topic->Prototype())
                {
                my $tooltipID = $self->BuildToolTip($topic->Symbol(), $sourceFile, $topic->Type(),
                                                                     $topic->Prototype(), $topic->Summary());
                $toolTipProperties = $self->BuildToolTipLinkProperties($tooltipID);
                };

            $output .=
            '<a href="#' . $self->SymbolToHTMLSymbol($parsedFile->[$index]->Symbol()) . '" ' . $toolTipProperties . '>'
                . $self->StringToHTML( $parsedFile->[$index]->Title(), ADD_HIDDEN_BREAKS)
            . '</a>';


            # Close the modifiers.

            if ($indent)
                {  $output .= '</div>';  };

            $output .=
                    '</div>' # Entry
                . '</div>' # Type

            . '</td><td' . $descriptionSizeAttr . '>'

                . '<div class=S' . ($index == 0 ? 'Main' : NaturalDocs::Topics->NameOfType($topic->Type(), 0, 1)) . '>'
                    . '<div class=SDescription>';


            # Add the modifiers to the HTML yet again.

            if ($indent)
                {  $output .= '<div class=SIndent' . $indent . '>';  };


            if (defined $parsedFile->[$index]->Body())
                {
                $output .= $self->NDMarkupToHTML($sourceFile, $parsedFile->[$index]->Summary(),
                                                                     $parsedFile->[$index]->Symbol(), $parsedFile->[$index]->Package(),
                                                                     $parsedFile->[$index]->Type(), $parsedFile->[$index]->Using());
                };


            # Close the modifiers again.

            if ($indent)
                {  $output .= '</div>';  };


            $output .=
                    '</div>' # Description
                . '</div>' # Type

            . '</td></tr>';


            # Prepare the modifiers for the next entry.

            if ($scope == ::SCOPE_START() || $scope == ::SCOPE_END())
                {
                $indent = 1;
                $inGroup = 0;
                }
            elsif ($topic->Type() eq ::TOPIC_GROUP())
                {
                if (!$inGroup)
                    {
                    $indent++;
                    $inGroup = 1;
                    };
                };

            if (!defined $isMarkedAttr)
                {  $isMarkedAttr = ' class=SMarked';  }
            else
                {  $isMarkedAttr = undef;  };

            $entrySizeAttr = undef;
            $descriptionSizeAttr = undef;

            $index++;
            };

        $output .=
        '</table>'
    . '</div>' # Border
    . '</div>' # Summary
    . "<!--END_ND_SUMMARY-->";

    return $output;
    };


#
#   Function: BuildPrototype
#
#   Builds and returns the prototype as HTML.
#
#   Parameters:
#
#       type - The <TopicType> the prototype is from.
#       prototype - The prototype to format.
#       file - The <FileName> the prototype was defined in.
#
#   Returns:
#
#       The prototype in HTML.
#
sub BuildPrototype #(type, prototype, file)
    {
    my ($self, $type, $prototype, $file) = @_;

    my $language = NaturalDocs::Languages->LanguageOf($file);
    my $prototypeObject = $language->ParsePrototype($type, $prototype);

    my $output;

    if ($prototypeObject->OnlyBeforeParameters())
        {
        $output =
        # A blockquote to scroll it if it's too long.
        '<blockquote>'
            # A surrounding table as a hack to make the div form-fit.
            . '<table border=0 cellspacing=0 cellpadding=0 class=Prototype><tr><td>'
                . $self->ConvertAmpChars($prototypeObject->BeforeParameters())
            . '</td></tr></table>'
        . '</blockquote>';
        }

    else
        {
        my $params = $prototypeObject->Parameters();
        my $beforeParams = $prototypeObject->BeforeParameters();
        my $afterParams = $prototypeObject->AfterParameters();


        # Determine what features the prototype has and its length.

        my ($hasType, $hasTypePrefix, $hasNamePrefix, $hasDefaultValue, $hasDefaultValuePrefix);
        my $maxParamLength = 0;

        foreach my $param (@$params)
            {
            my $paramLength = length($param->Name());

            if ($param->Type())
                {
                $hasType = 1;
                $paramLength += length($param->Type()) + 1;
                };
            if ($param->TypePrefix())
                {
                $hasTypePrefix = 1;
                $paramLength += length($param->TypePrefix()) + 1;
                };
            if ($param->NamePrefix())
                {
                $hasNamePrefix = 1;
                $paramLength += length($param->NamePrefix());
                };
            if ($param->DefaultValue())
                {
                $hasDefaultValue = 1;

                # The length of the default value part is either the longest word, or 1/3 the total, whichever is longer.  We do this
                # because we don't want parameter lines wrapping to more than three lines, and there's no guarantee that the line will
                # wrap at all.  There's a small possibility that it could still wrap to four lines with this code, but we don't need to go
                # crazy(er) here.

                my $thirdLength = length($param->DefaultValue()) / 3;

                my @words = split(/ +/, $param->DefaultValue());
                my $maxWordLength = 0;

                foreach my $word (@words)
                    {
                    if (length($word) > $maxWordLength)
                        {  $maxWordLength = length($word);  };
                    };

                $paramLength += ($maxWordLength > $thirdLength ? $maxWordLength : $thirdLength) + 1;
                };
            if ($param->DefaultValuePrefix())
                {
                $hasDefaultValuePrefix = 1;
                $paramLength += length($param->DefaultValuePrefix()) + 1;
                };

            if ($paramLength > $maxParamLength)
                {  $maxParamLength = $paramLength;  };
            };

        my $useCondensed = (length($beforeParams) + $maxParamLength + length($afterParams) > 80 ? 1 : 0);
        my $parameterColumns = 1 + $hasType + $hasTypePrefix + $hasNamePrefix +
                                               $hasDefaultValue + $hasDefaultValuePrefix + $useCondensed;

        $output =
        '<blockquote><table border=0 cellspacing=0 cellpadding=0 class=Prototype><tr><td>'

            # Stupid hack to get it to work right in IE.
            . '<table border=0 cellspacing=0 cellpadding=0><tr>'

            . '<td class=PBeforeParameters ' . ($useCondensed ? 'colspan=' . $parameterColumns : 'nowrap') . '>'
                . $self->ConvertAmpChars($beforeParams);

                if ($beforeParams && $beforeParams !~ /[\(\[\{\<]$/)
                    {  $output .= '&nbsp;';  };

            $output .=
            '</td>';

            for (my $i = 0; $i < scalar @$params; $i++)
                {
                if ($useCondensed)
                    {
                    $output .= '</tr><tr><td>&nbsp;&nbsp;&nbsp;</td>';
                    }
                elsif ($i > 0)
                    {
                    # Go to the next row and and skip the BeforeParameters cell.
                    $output .= '</tr><tr><td></td>';
                    };

                if ($language->TypeBeforeParameter())
                    {
                    if ($hasTypePrefix)
                        {
                        my $htmlTypePrefix = $self->ConvertAmpChars($params->[$i]->TypePrefix());
                        $htmlTypePrefix =~ s/ $/&nbsp;/;

                        $output .=
                        '<td class=PTypePrefix nowrap>'
                            . $htmlTypePrefix
                        . '</td>';
                        };

                    if ($hasType)
                        {
                        $output .=
                        '<td class=PType nowrap>'
                            . $self->ConvertAmpChars($params->[$i]->Type()) . '&nbsp;'
                        . '</td>';
                        };

                    if ($hasNamePrefix)
                        {
                        $output .=
                        '<td class=PParameterPrefix nowrap>'
                            . $self->ConvertAmpChars($params->[$i]->NamePrefix())
                        . '</td>';
                        };

                    $output .=
                    '<td class=PParameter nowrap' . ($useCondensed && !$hasDefaultValue ? ' width=100%' : '') . '>'
                        . $self->ConvertAmpChars($params->[$i]->Name())
                    . '</td>';
                    }

                else # !$language->TypeBeforeParameter()
                    {
                    $output .=
                    '<td class=PParameter nowrap>'
                        . $self->ConvertAmpChars( $params->[$i]->NamePrefix() . $params->[$i]->Name() )
                    . '</td>';

                    if ($hasType || $hasTypePrefix)
                        {
                        my $typePrefix = $params->[$i]->TypePrefix();
                        if ($typePrefix)
                            {  $typePrefix .= ' ';  };

                        $output .=
                        '<td class=PType nowrap' . ($useCondensed && !$hasDefaultValue ? ' width=100%' : '') . '>'
                            . '&nbsp;' . $self->ConvertAmpChars( $typePrefix . $params->[$i]->Type() )
                        . '</td>';
                        };
                    };

                if ($hasDefaultValuePrefix)
                    {
                    $output .=
                    '<td class=PDefaultValuePrefix>'
                        . '&nbsp;' . $self->ConvertAmpChars( $params->[$i]->DefaultValuePrefix() ) . '&nbsp;'
                    . '</td>';
                    };

                if ($hasDefaultValue)
                    {
                    $output .=
                    '<td class=PDefaultValue width=100%>'
                        . ($hasDefaultValuePrefix ? '' : '&nbsp;') . $self->ConvertAmpChars( $params->[$i]->DefaultValue() )
                    . '</td>';
                    };
                };

            if ($useCondensed)
                {  $output .= '</tr><tr>';  };

            $output .=
            '<td class=PAfterParameters ' . ($useCondensed ? 'colspan=' . $parameterColumns : 'nowrap') . '>'
                 . $self->ConvertAmpChars($afterParams);

                if ($afterParams && $afterParams !~ /^[\)\]\}\>]/)
                    {  $output .= '&nbsp;';  };

            $output .=
            '</td>'
        . '</tr></table>'

        # Hack.
        . '</td></tr></table></blockquote>';
       };

    return $output;
    };


#
#   Function: BuildFooter
#
#   Builds and returns the HTML footer for the page.
#
sub BuildFooter
    {
    my $self = shift;
    my $footer = NaturalDocs::Menu->Footer();

    if (defined $footer)
        {
        if (substr($footer, -1, 1) ne '.')
            {  $footer .= '.';  };

        $footer =~ s/\(c\)/&copy;/gi;
        $footer =~ s/\(tm\)/&trade;/gi;
        $footer =~ s/\(r\)/&reg;/gi;

        $footer .= '&nbsp; Generated by <a href="' . NaturalDocs::Settings->AppURL() . '">Natural Docs</a>.'
        }
    else
        {
        $footer = 'Generated by <a href="' . NaturalDocs::Settings->AppURL() . '">Natural Docs</a>';
        };

    return '<!--START_ND_FOOTER-->' . $footer . '<!--END_ND_FOOTER-->';
    };


#
#   Function: BuildToolTip
#
#   Builds the HTML for a symbol's tooltip and stores it in <tooltipHTML>.
#
#   Parameters:
#
#       symbol - The target <SymbolString>.
#       file - The <FileName> the target's defined in.
#       type - The symbol <TopicType>.
#       prototype - The target prototype, or undef for none.
#       summary - The target summary, or undef for none.
#
#   Returns:
#
#       If a tooltip is necessary for the link, returns the tooltip ID.  If not, returns undef.
#
sub BuildToolTip #(symbol, file, type, prototype, summary)
    {
    my ($self, $symbol, $file, $type, $prototype, $summary) = @_;

    if (defined $prototype || defined $summary)
        {
        my $htmlSymbol = $self->SymbolToHTMLSymbol($symbol);
        my $number = $tooltipSymbolsToNumbers{$htmlSymbol};

        if (!defined $number)
            {
            $number = $tooltipNumber;
            $tooltipNumber++;

            $tooltipSymbolsToNumbers{$htmlSymbol} = $number;

            $tooltipHTML .=
            '<div class=CToolTip id="tt' . $number . '">'
                . '<div class=C' . NaturalDocs::Topics->NameOfType($type, 0, 1) . '>';

            if (defined $prototype)
                {
                $tooltipHTML .= $self->BuildPrototype($type, $prototype, $file);
                };

            if (defined $summary)
                {
                # Remove links, since people can't/shouldn't be clicking on tooltips anyway.
                $summary =~ s/<\/?(?:link|url)>//g;

                # The fact that we don't have scope or using shouldn't matter because we removed the links.
                $summary = $self->NDMarkupToHTML($file, $summary, undef, undef, $type, undef);

                # XXX - Hack.  We want to remove e-mail links as well, but keep their obfuscation.  So we leave the tags in there for
                # the NDMarkupToHTML call, then strip out the link part afterwards.  The text obfuscation should still be in place.

                $summary =~ s/<\/?a[^>]+>//g;

                $tooltipHTML .= $summary;
                };

            $tooltipHTML .=
                '</div>'
            . '</div>';
            };

        return 'tt' . $number;
        }
    else
        {  return undef;  };
    };

#
#   Function: BuildToolTips
#
#   Builds and returns the tooltips for the page in HTML.
#
sub BuildToolTips
    {
    my $self = shift;
    return "\n<!--START_ND_TOOLTIPS-->\n" . $tooltipHTML . "<!--END_ND_TOOLTIPS-->\n\n";
    };

#
#   Function: BuildClassHierarchy
#
#   Builds and returns a class hierarchy diagram for the passed class, if applicable.
#
#   Parameters:
#
#       file - The source <FileName>.
#       class - The class <SymbolString> to build the hierarchy of.
#
sub BuildClassHierarchy #(file, symbol)
    {
    my ($self, $file, $symbol) = @_;

    my @parents = NaturalDocs::ClassHierarchy->ParentsOf($symbol);
    @parents = sort { ::StringCompare($a, $b) } @parents;

    my @children = NaturalDocs::ClassHierarchy->ChildrenOf($symbol);
    @children = sort { ::StringCompare($a, $b) } @children;

    if (!scalar @parents && !scalar @children)
        {  return undef;  };

    my $output =
    '<div class=ClassHierarchy>';

    if (scalar @parents)
        {
        $output .='<table border=0 cellspacing=0 cellpadding=0><tr><td>';

        foreach my $parent (@parents)
            {
            $output .= $self->BuildClassHierarchyEntry($file, $parent, 'CHParent', 1);
            };

        $output .= '</td></tr></table><div class=CHIndent>';
        };

    $output .=
    '<table border=0 cellspacing=0 cellpadding=0><tr><td>'
        . $self->BuildClassHierarchyEntry($file, $symbol, 'CHCurrent', undef)
    . '</td></tr></table>';

    if (scalar @children)
        {
        $output .=
        '<div class=CHIndent>'
            . '<table border=0 cellspacing=0 cellpadding=0><tr><td>';

        if (scalar @children <= 5)
            {
            for (my $i = 0; $i < scalar @children; $i++)
                {  $output .= $self->BuildClassHierarchyEntry($file, $children[$i], 'CHChild', 1);  };
            }
        else
            {
            for (my $i = 0; $i < 4; $i++)
                {  $output .= $self->BuildClassHierarchyEntry($file, $children[$i], 'CHChild', 1);  };

           $output .= '<div class=CHChildNote><div class=CHEntry>' . (scalar @children - 4) . ' other children</div></div>';
            };

        $output .=
        '</td></tr></table>'
        . '</div>';
        };

    if (scalar @parents)
        {  $output .= '</div>';  };

    $output .=
    '</div>';

    return $output;
    };


#
#   Function: BuildClassHierarchyEntry
#
#   Builds and returns a single class hierarchy entry.
#
#   Parameters:
#
#       file - The source <FileName>.
#       symbol - The class <SymbolString> whose entry is getting built.
#       style - The style to apply to the entry, such as <CHParent>.
#       link - Whether to build a link for this class or not.  When building the selected class' entry, this should be false.  It will
#               automatically handle whether the symbol is defined or not.
#
sub BuildClassHierarchyEntry #(file, symbol, style, link)
    {
    my ($self, $file, $symbol, $style, $link) = @_;

    my @identifiers = NaturalDocs::SymbolString->IdentifiersOf($symbol);
    my $name = join(NaturalDocs::Languages->LanguageOf($file)->PackageSeparator(), @identifiers);
    $name = $self->StringToHTML($name);

    my $output = '<div class=' . $style . '><div class=CHEntry>';

    if ($link)
        {
        my $target = NaturalDocs::SymbolTable->Lookup($symbol, $file);

        if (defined $target)
            {
            my $targetFile;

            if ($target->File() ne $file)
                {  $targetFile = $self->MakeRelativeURL( $self->OutputFileOf($file), $self->OutputFileOf($target->File()), 1 );  };
            # else leave it undef

            my $targetTooltipID = $self->BuildToolTip($symbol, $target->File(), $target->Type(),
                                                                          $target->Prototype(), $target->Summary());

            my $toolTipProperties = $self->BuildToolTipLinkProperties($targetTooltipID);

            $output .= '<a href="' . $targetFile . '#' . $self->SymbolToHTMLSymbol($symbol) . '" '
                            . 'class=L' . NaturalDocs::Topics->NameOfType($target->Type(), 0, 1) . ' ' . $toolTipProperties . '>'
                            . $name . '</a>';
            }
        else
            {  $output .= $name;  };
        }
    else
        {  $output .= $name;  };

    $output .= '</div></div>';
    return $output;
    };


#
#   Function: OpeningBrowserStyles
#
#   Returns the JavaScript that will add opening browser styles if necessary.
#
sub OpeningBrowserStyles
    {
    my $self = shift;

    return

    '<script language=JavaScript><!--' . "\n"

        # IE 4 and 5 don't understand 'undefined', so you can't say '!= undefined'.
        . 'if (browserType) {'
            . 'document.write("<div class=" + browserType + ">");'
            . 'if (browserVer) {'
                . 'document.write("<div class=" + browserVer + ">"); }'
            . '}'

    . '// --></script>';
    };


#
#   Function: ClosingBrowserStyles
#
#   Returns the JavaScript that will close browser styles if necessary.
#
sub ClosingBrowserStyles
    {
    my $self = shift;

    return

    '<script language=JavaScript><!--' . "\n"

        . 'if (browserType) {'
            . 'if (browserVer) {'
                . 'document.write("</div>"); }'
            . 'document.write("</div>");'
            . '}'

    . '// --></script>';
    };


#
#   Function: StandardComments
#
#   Returns the standard HTML comments that should be included in every generated file.  This includes <IEWebMark()>, so this
#   really is required for proper functionality.
#
sub StandardComments
    {
    my $self = shift;

    return "\n\n"

        . '<!--  Generated by Natural Docs, version ' . NaturalDocs::Settings->TextAppVersion() . ' -->' . "\n"
        . '<!--  ' . NaturalDocs::Settings->AppURL() . '  -->' . "\n\n"
        . $self->IEWebMark() . "\n\n";
    };


#
#   Function: IEWebMark
#
#   Returns the HTML comment necessary to get around the security warnings in IE starting with Windows XP Service Pack 2.
#
#   With this mark, the HTML page is treated as if it were in the Internet security zone instead of the Local Machine zone.  This
#   prevents the lockdown on scripting that causes an error message to appear with each page.
#
#   More Information:
#
#       - http://www.microsoft.com/technet/prodtechnol/winxppro/maintain/sp2brows.mspx#EHAA
#       - http://www.phdcc.com/xpsp2.htm#markoftheweb
#
sub IEWebMark
    {
    my $self = shift;

    return '<!-- saved from url=(0026)http://www.naturaldocs.org -->';
    };



###############################################################################
# Group: Index Functions


#
#   Function: BuildIndexPages
#
#   Builds an index file or files.
#
#   Parameters:
#
#       type - The <TopicType> the index is limited to, or undef for none.
#       index  - An arrayref of sections, each section being an arrayref <NaturalDocs::SymbolTable::IndexElement> objects.
#                   The first section is for symbols, the second for numbers, and the rest for A through Z.
#       beginPage - All the content of the HTML page up to where the index content should appear.
#       endPage - All the content of the HTML page past where the index should appear.
#
#   Returns:
#
#       The number of pages in the index.
#
sub BuildIndexPages #(type, index, beginPage, endPage)
    {
    my ($self, $type, $indexSections, $beginPage, $endPage) = @_;

    # Build the content.

    my ($indexHTMLSections, $tooltipHTMLSections) = $self->BuildIndexSections($indexSections, $self->IndexFileOf($type, 1));


    my $page = 1;
    my $pageSize = 0;
    my @pageLocations;

    # The maximum page size acceptable before starting a new page.  Note that this doesn't include beginPage and endPage,
    # because we don't want something like a large menu screwing up the calculations.
    use constant PAGESIZE_LIMIT => 50000;


    # File the pages.

    for (my $i = 0; $i < scalar @$indexHTMLSections; $i++)
        {
        if (!defined $indexHTMLSections->[$i])
            {  next;  };

        $pageSize += length($indexHTMLSections->[$i]) + length($tooltipHTMLSections->[$i]);
        $pageLocations[$i] = $page;

        if ($pageSize + length($indexHTMLSections->[$i+1]) + length($tooltipHTMLSections->[$i+1]) > PAGESIZE_LIMIT)
            {
            $page++;
            $pageSize = 0;
            };
        };


    # Build the pages.

    my $indexFileName;
    $page = -1;
    my $oldPage = -1;
    my $tooltips;
    my $firstHeading;

    for (my $i = 0; $i < scalar @$indexHTMLSections; $i++)
        {
        if (!defined $indexHTMLSections->[$i])
            {  next;  };

        $page = $pageLocations[$i];

        # Switch files if we need to.

        if ($page != $oldPage)
            {
            if ($oldPage != -1)
                {
                print INDEXFILEHANDLE '</table>' . $tooltips . $endPage;
                close(INDEXFILEHANDLE);
                $tooltips = undef;
                };

            $indexFileName = $self->IndexFileOf($type, $page);

            open(INDEXFILEHANDLE, '>' . $indexFileName)
                or die "Couldn't create output file " . $indexFileName . ".\n";

            print INDEXFILEHANDLE $beginPage . $self->BuildIndexNavigationBar($type, $page, \@pageLocations)
                                              . '<table border=0 cellspacing=0 cellpadding=0>';

            $oldPage = $page;
            $firstHeading = 1;
            };

        print INDEXFILEHANDLE
        '<tr>'
            . '<td class=IHeading' . ($firstHeading ? ' id=IFirstHeading' : '') . '>'
                . '<a name="' . $indexAnchors[$i] . '"></a>'
                 . $indexHeadings[$i]
            . '</td>'
            . '<td></td>'
        . '</tr>'

        . $indexHTMLSections->[$i];

        $firstHeading = 0;
        $tooltips .= $tooltipHTMLSections->[$i];
        };

    if ($page != -1)
        {
        print INDEXFILEHANDLE '</table>' . $tooltips . $endPage;
        close(INDEXFILEHANDLE);
        }

    # Build a dummy page so there's something at least.
    else
        {
        $indexFileName = $self->IndexFileOf($type, 1);

        open(INDEXFILEHANDLE, '>' . $indexFileName)
            or die "Couldn't create output file " . $indexFileName . ".\n";

        print INDEXFILEHANDLE
            $beginPage
            . $self->BuildIndexNavigationBar($type, 1, \@pageLocations)
            . 'There are no entries in the ' . lc( NaturalDocs::Topics->NameOfType($type) ) . ' index.'
            . $endPage;

        close(INDEXFILEHANDLE);
        };


    return $page;
    };


#
#   Function: BuildIndexSections
#
#   Builds and returns index's sections in HTML.
#
#   Parameters:
#
#       index  - An arrayref of sections, each section being an arrayref <NaturalDocs::SymbolTable::IndexElement> objects.
#                   The first section is for symbols, the second for numbers, and the rest for A through Z.
#       outputFile - The output file the index is going to be stored in.  Since there may be multiple files, just send the first file.  The
#                        path is what matters, not the file name.
#
#   Returns:
#
#       The arrayref ( indexSections, tooltipSections ).
#
#       Index 0 is the symbols, index 1 is the numbers, and each following index is A through Z.  The content of each section
#       is its HTML, or undef if there is nothing for that section.
#
sub BuildIndexSections #(index, outputFile)
    {
    my ($self, $indexSections, $outputFile) = @_;

    $self->ResetToolTips();

    my $contentSections = [ ];
    my $tooltipSections = [ ];

    for (my $section = 0; $section < scalar @$indexSections; $section++)
        {
        if (defined $indexSections->[$section])
            {
            my $total = scalar @{$indexSections->[$section]};

            for (my $i = 0; $i < $total; $i++)
                {
                my $id;

                if ($i == 0)
                    {
                    if ($total == 1)
                        {  $id = 'IOnlySymbolPrefix';  }
                    else
                        {  $id = 'IFirstSymbolPrefix';  };
                    }
                elsif ($i == $total - 1)
                    {  $id = 'ILastSymbolPrefix';  };

                $contentSections->[$section] .= $self->BuildIndexElement($indexSections->[$section]->[$i], $outputFile, $id);
                };

            $tooltipSections->[$section] .= $self->BuildToolTips();
            $self->ResetToolTips(1);
            };
        };


    return ( $contentSections, $tooltipSections );
    };


#
#   Function: BuildIndexElement
#
#   Converts a <NaturalDocs::SymbolTable::IndexElement> to HTML and returns it.  It will handle all sub-elements automatically.
#
#   Parameters:
#
#       element - The <NaturalDocs::SymbolTable::IndexElement> to build.
#       outputFile - The output <FileName> this is appearing in.
#       id - The CSS ID to apply to the prefix.
#
#   Recursion-Only Parameters:
#
#       These parameters are used internally for recursion, and should not be set.
#
#       symbol - If the element is below symbol level, the <SymbolString> to use.
#       package - If the element is below package level, the package <SymbolString> to use.
#       hasPackage - Whether the element is below package level.  Is necessary because package may need to be undef.
#
sub BuildIndexElement #(element, outputFile, id, symbol, package, hasPackage)
    {
    my ($self, $element, $outputFile, $id, $symbol, $package, $hasPackage) = @_;

    my $output;


    # If we're doing a file sub-index entry...

    if ($hasPackage)
        {
        my ($inputDirectory, $relativePath) = NaturalDocs::Settings->SplitFromInputDirectory($element->File());

        $output =
        $self->BuildIndexLink($self->StringToHTML($relativePath, ADD_HIDDEN_BREAKS), $symbol,
                                        $package, $element->File(), $element->Type(), $element->Prototype(),
                                        $element->Summary(), $outputFile, 'IFile')
        }


    # If we're doing a package sub-index entry...

    elsif (defined $symbol)
        {
        my $text;

        if ($element->Package())
            {
            $text = NaturalDocs::SymbolString->ToText($element->Package(), $element->PackageSeparator());
            $text = $self->StringToHTML($text, ADD_HIDDEN_BREAKS);
            }
        else
            {  $text = 'Global';  };

        if (!$element->HasMultipleFiles())
            {
            $output .= $self->BuildIndexLink($text, $symbol, $element->Package(), $element->File(), $element->Type(),
                                                             $element->Prototype(), $element->Summary(), $outputFile, 'IParent');
            }

        else
            {
            $output .=
            '<span class=IParent>'
                . $text
            . '</span>'
            . '<div class=ISubIndex>';

            my $fileElements = $element->File();
            foreach my $fileElement (@$fileElements)
                {
                $output .= $self->BuildIndexElement($fileElement, $outputFile, $id, $symbol, $element->Package(), 1);
                };

            $output .=
            '</div>';
            };
        }


    # If we're doing a top-level symbol entry...

    else
        {
        my $symbolText = $self->StringToHTML($element->SortableSymbol(), ADD_HIDDEN_BREAKS);
        my $symbolPrefix = $self->StringToHTML($element->IgnoredPrefix());

        $output .=
        '<tr>'
            . '<td class=ISymbolPrefix' . ($id ? ' id=' . $id : '') . '>'
                . ($symbolPrefix || '&nbsp;')
            . '</td><td class=IEntry>';

        if (!$element->HasMultiplePackages())
            {
            my $packageText;

            if (defined $element->Package())
                {
                $packageText = NaturalDocs::SymbolString->ToText($element->Package(), $element->PackageSeparator());
                $packageText = $self->StringToHTML($packageText, ADD_HIDDEN_BREAKS);
                };

            if (!$element->HasMultipleFiles())
                {
                $output .=
                    $self->BuildIndexLink($symbolText, $element->Symbol(), $element->Package(), $element->File(),
                                                     $element->Type(), $element->Prototype(), $element->Summary(), $outputFile, 'ISymbol');

                if (defined $packageText)
                    {
                    $output .=
                    ', <span class=IParent>'
                        . $packageText
                    . '</span>';
                    };
                }
            else # hasMultipleFiles but not mulitplePackages
                {
                $output .=
                '<span class=ISymbol>'
                    . $symbolText
                . '</span>';

                if (defined $packageText)
                    {
                    $output .=
                    ', <span class=IParent>'
                        . $packageText
                    . '</span>';
                    };

                $output .=
                '<div class=ISubIndex>';

                my $fileElements = $element->File();
                foreach my $fileElement (@$fileElements)
                    {
                    $output .= $self->BuildIndexElement($fileElement, $outputFile, $id, $element->Symbol(), $element->Package(), 1);
                    };

                $output .=
                '</div>';
                };
            }

        else # hasMultiplePackages
            {
            $output .=
            '<span class=ISymbol>'
                . $symbolText
            . '</span>'
            . '<div class=ISubIndex>';

            my $packageElements = $element->Package();
            foreach my $packageElement (@$packageElements)
                {
                $output .= $self->BuildIndexElement($packageElement, $outputFile, $id, $element->Symbol());
                };

            $output .=
            '</div>';
            };

        $output .= '</td></tr>';
        };


    return $output;
    };


#
#   Function: BuildIndexLink
#
#   Builds and returns the HTML associated with an index link.  The HTML will be the a href tag, the text, and the closing tag.
#
#   Parameters:
#
#       text - The text of the link *in HTML*.  Use <IndexSymbolToHTML()> if necessary.
#       symbol - The partial <SymbolString> to link to.
#       package - The package <SymbolString> of the symbol.
#       file - The <FileName> the symbol is defined in.
#       type - The <TopicType> of the symbol.
#       prototype - The prototype of the symbol, or undef if none.
#       summary - The summary of the symbol, or undef if none.
#       outputFile - The HTML <FileName> this link will appear in.
#       style - The CSS style to apply to the link.
#
sub BuildIndexLink #(text, symbol, package, file, type, prototype, summary, outputFile, style)
    {
    my ($self, $text, $symbol, $package, $file, $type, $prototype, $summary, $outputFile, $style) = @_;

    $symbol = NaturalDocs::SymbolString->Join($package, $symbol);

    my $targetTooltipID = $self->BuildToolTip($symbol, $file, $type, $prototype, $summary);
    my $toolTipProperties = $self->BuildToolTipLinkProperties($targetTooltipID);

    return '<a href="' . $self->MakeRelativeURL( $outputFile, $self->OutputFileOf($file), 1 )
                         . '#' . $self->SymbolToHTMLSymbol($symbol) . '" ' . $toolTipProperties . ' '
                . 'class=' . $style . '>' . $text . '</a>';
    };


#
#   Function: BuildIndexNavigationBar
#
#   Builds a navigation bar for a page of the index.
#
#   Parameters:
#
#       type - The <TopicType> of the index, or undef for general.
#       page - The page of the index the navigation bar is for.
#       locations - An arrayref of the locations of each section.  Index 0 is for the symbols, index 1 for the numbers, and the rest
#                       for each letter.  The values are the page numbers where the sections are located.
#
sub BuildIndexNavigationBar #(type, page, locations)
    {
    my ($self, $type, $page, $locations) = @_;

    my $output = '<div class=INavigationBar>';

    for (my $i = 0; $i < scalar @indexHeadings; $i++)
        {
        if ($i != 0)
            {  $output .= ' &middot; ';  };

        if (defined $locations->[$i])
            {
            $output .= '<a href="';

            if ($locations->[$i] != $page)
                {  $output .= $self->RelativeIndexFileOf($type, $locations->[$i]);  };

            $output .= '#' . $indexAnchors[$i] . '">' . $indexHeadings[$i] . '</a>';
            }
        else
            {
            $output .= $indexHeadings[$i];
            };
        };

    $output .= '</div>';

    return $output;
    };



###############################################################################
# Group: File Functions


#
#   function: PurgeIndexFiles
#
#   Removes all or some of the output files for an index.
#
#   Parameters:
#
#       type  - The index <TopicType>.
#       startingPage - If defined, only pages starting with this number will be removed.  Otherwise all pages will be removed.
#
sub PurgeIndexFiles #(type, startingPage)
    {
    my ($self, $type, $page) = @_;

    if (!defined $page)
        {  $page = 1;  };

    for (;;)
        {
        my $file = $self->IndexFileOf($type, $page);

        if (-e $file)
            {
            unlink($file);
            $page++;
            }
        else
            {
            last;
            };
        };
    };


#
#   function: OutputFileOf
#
#   Returns the output file name of the source file.  Will be undef if it is not a file from a valid input directory.
#
sub OutputFileOf #(sourceFile)
    {
    my ($self, $sourceFile) = @_;

    my ($inputDirectory, $relativeSourceFile) = NaturalDocs::Settings->SplitFromInputDirectory($sourceFile);
    if (!defined $inputDirectory)
        {  return undef;  };

    my $outputDirectory = NaturalDocs::Settings->OutputDirectoryOf($self);
    my $inputDirectoryName = NaturalDocs::Settings->InputDirectoryNameOf($inputDirectory);

    $outputDirectory = NaturalDocs::File->JoinPaths( $outputDirectory,
                                                                            'files' . ($inputDirectoryName != 1 ? $inputDirectoryName : ''), 1 );

    # We need to change any extensions to dashes because Apache will think file.pl.html is a script.
    # We also need to add a dash if the file doesn't have an extension so there'd be no conflicts with index.html,
    # FunctionIndex.html, etc.

    if (!($relativeSourceFile =~ tr/./-/))
        {  $relativeSourceFile .= '-';  };

    $relativeSourceFile =~ tr/ /_/;
    $relativeSourceFile .= '.html';

    return NaturalDocs::File->JoinPaths($outputDirectory, $relativeSourceFile);
    };


#
#   Function: IndexDirectory
#
#   Returns the directory of the index files.
#
sub IndexDirectory
    {
    my $self = shift;
    return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->OutputDirectoryOf($self), 'index', 1);
    };


#
#   function: IndexFileOf
#
#   Returns the output file name of the index file.
#
#   Parameters:
#
#       type  - The <TopicType> of the index.
#       page  - The page number.  Undef is the same as one.
#
sub IndexFileOf #(type, page)
    {
    my ($self, $type, $page) = @_;
    return NaturalDocs::File->JoinPaths( $self->IndexDirectory(), $self->RelativeIndexFileOf($type, $page) );
    };

#
#   function: RelativeIndexFileOf
#
#   Returns the output file name of the index file, relative to other index files.
#
#   Parameters:
#
#       type  - The <TopicType> of the index.
#       page  - The page number.  Undef is the same as one.
#
sub RelativeIndexFileOf #(type, page)
    {
    my ($self, $type, $page) = @_;
    return NaturalDocs::Topics->NameOfType($type, 1, 1) . (defined $page && $page != 1 ? $page : '') . '.html';
    };


#
#   function: CSSDirectory
#
#   Returns the directory of the CSS files.
#
sub CSSDirectory
    {
    my $self = shift;
    return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->OutputDirectoryOf($self), 'styles', 1);
    };


#
#   Function: MainCSSFile
#
#   Returns the location of the main CSS file.
#
sub MainCSSFile
    {
    my $self = shift;
    return NaturalDocs::File->JoinPaths( $self->CSSDirectory(), 'main.css' );
    };


#
#   function: JavaScriptDirectory
#
#   Returns the directory of the JavaScript files.
#
sub JavaScriptDirectory
    {
    my $self = shift;
    return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->OutputDirectoryOf($self), 'javascript', 1);
    };


#
#   Function: MainJavaScriptFile
#
#   Returns the location of the main JavaScript file.
#
sub MainJavaScriptFile
    {
    my $self = shift;
    return NaturalDocs::File->JoinPaths( $self->JavaScriptDirectory(), 'main.js' );
    };




###############################################################################
# Group: Support Functions


#
#   function:IndexTitleOf
#
#   Returns the page title of the index file.
#
#   Parameters:
#
#       type  - The type of index.
#
sub IndexTitleOf #(type)
    {
    my ($self, $type) = @_;

    return ($type eq ::TOPIC_GENERAL() ? '' : NaturalDocs::Topics->NameOfType($type) . ' ') . 'Index';
    };

#
#   function: MakeRelativeURL
#
#   Returns a relative path between two files in the output tree and returns it in URL format.
#
#   Parameters:
#
#       baseFile    - The base <FileName> in local format, *not* in URL format.
#       targetFile  - The target <FileName> of the link in local format, *not* in URL format.
#       baseHasFileName - Whether baseFile has a file name attached or is just a path.
#
#   Returns:
#
#       The relative URL to the target.
#
sub MakeRelativeURL #(FileName baseFile, FileName targetFile, bool baseHasFileName) -> string relativeURL
    {
    my ($self, $baseFile, $targetFile, $baseHasFileName) = @_;

    if ($baseHasFileName)
        {  $baseFile = NaturalDocs::File->NoFileName($baseFile)  };

    my $relativePath = NaturalDocs::File->MakeRelativePath($baseFile, $targetFile);

    return $self->ConvertAmpChars( NaturalDocs::File->ConvertToURL($relativePath) );
    };

#
#   Function: StringToHTML
#
#   Converts a text string to HTML.  Does not apply paragraph tags or accept formatting tags.
#
#   Parameters:
#
#       string - The string to convert.
#       addHiddenBreaks - Whether to add hidden breaks to the string.  You can use <ADD_HIDDEN_BREAKS> for this parameter
#                                   if you want to make the calling code clearer.
#
#   Returns:
#
#       The string in HTML.
#
sub StringToHTML #(string, addHiddenBreaks)
    {
    my ($self, $string, $addHiddenBreaks) = @_;

    $string =~ s/&/&amp;/g;
    $string =~ s/</&lt;/g;
    $string =~ s/>/&gt;/g;

    # Me likey the fancy quotes.  They work in IE 4+, Mozilla, and Opera 5+.  We've already abandoned NS4 with the CSS
    # styles, so might as well.
    $string =~ s/^\'/&lsquo;/gm;
    $string =~ s/([\ \(\[\{])\'/$1&lsquo;/g;
    $string =~ s/\'/&rsquo;/g;

    $string =~ s/^\"/&ldquo;/gm;
    $string =~ s/([\ \(\[\{])\"/$1&ldquo;/g;
    $string =~ s/\"/&rdquo;/g;

    # Me likey the double spaces too.  As you can probably tell, I like print-formatting better than web-formatting.  The indented
    # paragraphs without blank lines in between them do become readable when you have fancy quotes and double spaces too.
    $string = $self->AddDoubleSpaces($string);

    if ($addHiddenBreaks)
        {  $string = $self->AddHiddenBreaks($string);  };

    return $string;
    };


#
#   Function: SymbolToHTMLSymbol
#
#   Converts a <SymbolString> to a HTML symbol, meaning one that is safe to include in anchor and link tags.  You don't need
#   to pass the result to <ConvertAmpChars()>.
#
sub SymbolToHTMLSymbol #(symbol)
    {
    my ($self, $symbol) = @_;

    my @identifiers = NaturalDocs::SymbolString->IdentifiersOf($symbol);
    my $htmlSymbol = join('.', @identifiers);

    # If only Mozilla was nice about putting special characters in URLs like IE and Opera are, I could leave spaces in and replace
    # "<>& with their amp chars.  But alas, Mozilla shows them as %20, etc. instead.  It would have made for nice looking URLs.
    $htmlSymbol =~ tr/ \"<>\?&%/_/d;

    return $htmlSymbol;
    };


#
#   Function: NDMarkupToHTML
#
#   Converts a block of <NDMarkup> to HTML.
#
#   Parameters:
#
#       sourceFile - The source <FileName> the <NDMarkup> appears in.
#       text    - The <NDMarkup> text to convert.
#       symbol - The topic <SymbolString> the <NDMarkup> appears in.
#       package  - The package <SymbolString> the <NDMarkup> appears in.
#       type - The <TopicType> the <NDMarkup> appears in.
#       using - An arrayref of scope <SymbolStrings> the <NDMarkup> also has access to, or undef if none.
#
#   Returns:
#
#       The text in HTML.
#
sub NDMarkupToHTML #(sourceFile, text, symbol, package, type, using)
    {
    my ($self, $sourceFile, $text, $symbol, $package, $type, $using) = @_;

    my $dlSymbolBehavior;

    if ($type == ::TOPIC_ENUMERATION())
        {  $dlSymbolBehavior = NaturalDocs::Languages->LanguageOf($sourceFile)->EnumValues();  }
    elsif (NaturalDocs::Topics->TypeInfo($type)->Scope() == ::SCOPE_ALWAYS_GLOBAL())
        {  $dlSymbolBehavior = ::ENUM_GLOBAL();  }
    else
        {  $dlSymbolBehavior = ::ENUM_UNDER_PARENT();  };

    my $output;
    my $inCode;

    my @splitText = split(/(<\/?code>)/, $text);

    while (scalar @splitText)
        {
        $text = shift @splitText;

        if ($text eq '<code>')
            {
            $output .= '<blockquote><pre class=CCode>';
            $inCode = 1;
            }
        elsif ($text eq '</code>')
            {
            $output .= '</pre></blockquote>';
            $inCode = undef;
            }
        elsif ($inCode)
            {
            $text =~ s/\n/<br>/g;
            $output .= $text;
            }
        else
            {
            # Format non-code text.

            # Convert quotes to fancy quotes.
            $text =~ s/^\'/&lsquo;/gm;
            $text =~ s/([\ \(\[\{])\'/$1&lsquo;/g;
            $text =~ s/\'/&rsquo;/g;

            $text =~ s/^&quot;/&ldquo;/gm;
            $text =~ s/([\ \(\[\{])&quot;/$1&ldquo;/g;
            $text =~ s/&quot;/&rdquo;/g;

            # Copyright symbols.  Prevent conversion when part of (a), (b), (c) lists.
            if ($text !~ /\(a\)/i)
                {  $text =~ s/\(c\)/&copy;/gi;  };

            # Trademark symbols.
            $text =~ s/\(tm\)/&trade;/gi;
            $text =~ s/\(r\)/&reg;/gi;

            # Resolve and convert links.
            $text =~ s/<link>([^<]+)<\/link>/$self->BuildTextLink($1, $package, $using, $sourceFile)/ge;
            $text =~ s/<url>([^<]+)<\/url>/$self->BuildURLLink($1)/ge;
            $text =~ s/<email>([^<]+)<\/email>/$self->BuildEMailLink($1)/eg;

            # Add double spaces too.
            $text = $self->AddDoubleSpaces($text);

            # Paragraphs
            $text =~ s/<p>/<p class=CParagraph>/g;

            # Bulleted lists
            $text =~ s/<ul>/<ul class=CBulletList>/g;

            # Headings
            $text =~ s/<h>/<h4 class=CHeading>/g;
            $text =~ s/<\/h>/<\/h4>/g;

            # Description Lists
            $text =~ s/<dl>/<table border=0 cellspacing=0 cellpadding=0 class=CDescriptionList>/g;
            $text =~ s/<\/dl>/<\/table>/g;

            $text =~ s/<de>/<tr><td class=CDLEntry>/g;
            $text =~ s/<\/de>/<\/td>/g;

            if ($dlSymbolBehavior == ::ENUM_GLOBAL())
                {  $text =~ s/<ds>([^<]+)<\/ds>/$self->MakeDescriptionListSymbol(undef, $1)/ge;  }
            elsif ($dlSymbolBehavior == ::ENUM_UNDER_PARENT())
                {  $text =~ s/<ds>([^<]+)<\/ds>/$self->MakeDescriptionListSymbol($package, $1)/ge;  }
            else # ($dlSymbolBehavior == ::ENUM_UNDER_TYPE())
                {  $text =~ s/<ds>([^<]+)<\/ds>/$self->MakeDescriptionListSymbol($symbol, $1)/ge;  }

            sub MakeDescriptionListSymbol #(package, text)
                {
                my ($self, $package, $text) = @_;

                $text = NaturalDocs::NDMarkup->RestoreAmpChars($text);
                my $symbol = NaturalDocs::SymbolString->FromText($text);

                if (defined $package)
                    {  $symbol = NaturalDocs::SymbolString->Join($package, $symbol);  };

                return
                '<tr>'
                    . '<td class=CDLEntry>'
                        # The anchors are closed, but not around the text, to prevent the :hover CSS style from kicking in.
                        . '<a name="' . $self->SymbolToHTMLSymbol($symbol) . '"></a>'
                        . $text
                    . '</td>';
                };

            $text =~ s/<dd>/<td class=CDLDescription>/g;
            $text =~ s/<\/dd>/<\/td><\/tr>/g;

            $output .= $text;
            };
        };

    return $output;
    };


#
#   Function: BuildTextLink
#
#   Creates a HTML link to a symbol, if it exists.
#
#   Parameters:
#
#       text  - The link text
#       package  - The package <SymbolString> the link appears in, or undef if none.
#       using - An arrayref of additional scope <SymbolStrings> the link has access to, or undef if none.
#       sourceFile  - The <FileName> the link appears in.
#
#   Returns:
#
#       The link in HTML, including tags.  If the link doesn't resolve to anything, returns the HTML that should be substituted for it.
#
sub BuildTextLink #(text, package, using, sourceFile)
    {
    my ($self, $text, $package, $using, $sourceFile) = @_;

    my $plainText = $self->RestoreAmpChars($text);

    my $symbol = NaturalDocs::SymbolString->FromText($plainText);
    my $target = NaturalDocs::SymbolTable->References(::REFERENCE_TEXT(), $symbol, $package, $using, $sourceFile);

    if (defined $target)
        {
        my $targetFile;

        if ($target->File() ne $sourceFile)
            {  $targetFile = $self->MakeRelativeURL( $self->OutputFileOf($sourceFile), $self->OutputFileOf($target->File()), 1 );  };
        # else leave it undef

        my $targetTooltipID = $self->BuildToolTip($target->Symbol(), $sourceFile, $target->Type(),
                                                                      $target->Prototype(), $target->Summary());

        my $toolTipProperties = $self->BuildToolTipLinkProperties($targetTooltipID);

        return '<a href="' . $targetFile . '#' . $self->SymbolToHTMLSymbol($target->Symbol()) . '" '
                    . 'class=L' . NaturalDocs::Topics->NameOfType($target->Type(), 0, 1) . ' ' . $toolTipProperties . '>' . $text . '</a>';
        }
    else
        {
        return '&lt;' . $text . '&gt;';
        };
    };


#
#   Function: BuildURLLink
#
#   Creates a HTML link to an external URL.  Long URLs will have hidden breaks to allow them to wrap.
#
#   Parameters:
#
#       url - The URL to link to.
#
#   Returns:
#
#       The HTML link, complete with tags.
#
sub BuildURLLink #(url)
    {
    my ($self, $url) = @_;

    $url = $self->RestoreAmpChars($url);

    if (length $url < 50)
        {  return '<a href="' . $url . '" class=LURL>' . $self->ConvertAmpChars($url) . '</a>';  };

    my @segments = split(/([\,\&\/])/, $url);
    my $output = '<a href="' . $url . '" class=LURL>';

    # Get past the first batch of slashes, since we don't want to break on things like http://.

    $output .= $self->ConvertAmpChars($segments[0]);

    my $i = 1;
    while ($i < scalar @segments && ($segments[$i] eq '/' || !$segments[$i]))
        {
        $output .= $segments[$i];
        $i++;
        };

    # Now break on each one of those symbols.

    while ($i < scalar @segments)
        {
        # Spaces don't wrap in IE for some reason.  Need to use dashes as well.
        if ($segments[$i] eq ',' || $segments[$i] eq '/' || $segments[$i] eq '&')
            {  $output .= '<span class=HB>- </span>';  };

        $output .= $self->ConvertAmpChars($segments[$i]);
        $i++;
        };

    $output .= '</a>';
    return $output;
    };


#
#   Function: BuildEMailLink
#
#   Creates a HTML link to an e-mail address.  The address will be transparently munged to protect it (hopefully) from spambots.
#
#   Parameters:
#
#       address  - The e-mail address.
#
#   Returns:
#
#       The HTML e-mail link, complete with tags.
#
sub BuildEMailLink #(address)
    {
    my ($self, $address) = @_;
    my @splitAddress;


    # Hack the address up.  We want two user pieces and two host pieces.

    my ($user, $host) = split(/\@/, $address);

    my $userSplit = length($user) / 2;

    push @splitAddress, substr($user, 0, $userSplit);
    push @splitAddress, substr($user, $userSplit);

    push @splitAddress, '@';

    my $hostSplit = length($host) / 2;

    push @splitAddress, substr($host, 0, $hostSplit);
    push @splitAddress, substr($host, $hostSplit);


    # Now put it back together again.  We'll use spans to split the text transparently and JavaScript to split and join the link.

    return
    "<a href=\"#\" onClick=\"location.href='mai' + 'lto:' + '" . join("' + '", @splitAddress) . "'; return false;\" class=LEMail>"
        . $splitAddress[0] . '<span style="display: none">.nosp@m.</span>' . $splitAddress[1]
        . '<span>@</span>'
        . $splitAddress[3] . '<span style="display: none">.nosp@m.</span>' . $splitAddress[4]
    . '</a>';
    };


#
#   Function: BuildToolTipLinkProperties
#
#   Returns the properties that should go in the link tag to add a tooltip to it.  Because the function accepts undef, you can
#   call it without checking if <BuildToolTip()> returned undef or not.
#
#   Parameters:
#
#       toolTipID - The ID of the tooltip.  If undef, the function will return undef.
#
#   Returns:
#
#       The properties that should be put in the link tag, or undef if toolTipID wasn't specified.
#
sub BuildToolTipLinkProperties #(toolTipID)
    {
    my ($self, $toolTipID) = @_;

    if (defined $toolTipID)
        {
        my $currentNumber = $tooltipLinkNumber;
        $tooltipLinkNumber++;

        return 'id=link' . $currentNumber . ' '
                . 'onMouseOver="ShowTip(event, \'' . $toolTipID . '\', \'link' . $currentNumber . '\')" '
                . 'onMouseOut="HideTip(\'' . $toolTipID . '\')"';
        }
    else
        {  return undef;  };
    };


#
#   Function: AddDoubleSpaces
#
#   Adds second spaces after the appropriate punctuation with &nbsp; so they show up in HTML.  They don't occur if there isn't at
#   least one space after the punctuation, so things like class.member notation won't be affected.
#
#   Parameters:
#
#       text - The text to convert.
#
#   Returns:
#
#       The text with double spaces as necessary.
#
sub AddDoubleSpaces #(text)
    {
    my ($self, $text) = @_;

    # Question marks and exclamation points get double spaces unless followed by a lowercase letter.

    $text =~ s/  ([^\ \t\r\n] [\!\?])  # Must appear after a non-whitespace character to apply.

                      (&quot;|&[lr][sd]quo;|[\'\"\]\}\)]?)  # Tolerate closing quotes, parenthesis, etc.
                      ((?:<[^>]+>)*)  # Tolerate tags

                      \   # The space
                      (?![a-z])  # Not followed by a lowercase character.

                   /$1$2$3&nbsp;\ /gx;


    # Periods get double spaces if it's not followed by a lowercase letter.  However, if it's followed by a capital letter and the
    # preceding word is in the list of acceptable abbreviations, it won't get the double space.  Yes, I do realize I am seriously
    # over-engineering this.

    $text =~ s/  ([^\ \t\r\n]+)  # The word prior to the period.

                      \.

                      (&quot;|&[lr][sd]quo;|[\'\"\]\}\)]?)  # Tolerate closing quotes, parenthesis, etc.
                      ((?:<[^>]+>)*)  # Tolerate tags

                      \   # The space
                      ([^a-z])   # The next character, if it's not a lowercase letter.

                  /$1 . '.' . $2 . $3 . MaybeExpand($1, $4) . $4/gex;

    sub MaybeExpand #(leadWord, nextLetter)
        {
        my ($leadWord, $nextLetter) = @_;

        if ($nextLetter =~ /^[A-Z]$/ && exists $abbreviations{ lc($leadWord) } )
            { return ' '; }
        else
            { return '&nbsp; '; };
        };

    return $text;
    };


#
#   Function: ConvertAmpChars
#
#   Converts certain characters to their HTML amp char equivalents.
#
#   Parameters:
#
#       text - The text to convert.
#
#   Returns:
#
#       The converted text.
#
sub ConvertAmpChars #(text)
    {
    my ($self, $text) = @_;

    $text =~ s/&/&amp;/g;
    $text =~ s/\"/&quot;/g;
    $text =~ s/</&lt;/g;
    $text =~ s/>/&gt;/g;

    return $text;
    };


#
#   Function: RestoreAmpChars
#
#   Restores all amp characters to their original state.  This works with both <NDMarkup> amp chars and fancy quotes.
#
#   Parameters:
#
#       text - The text to convert.
#
#   Returns:
#
#       The converted text.
#
sub RestoreAmpChars #(text)
    {
    my ($self, $text) = @_;

    $text = NaturalDocs::NDMarkup->RestoreAmpChars($text);
    $text =~ s/&[lr]squo;/'/g;
    $text =~ s/&[lr]dquo;/"/g;

    return $text;
    };


#
#   Function: AddHiddenBreaks
#
#   Adds hidden breaks to symbols.  Puts them after symbol and directory separators so long names won't screw up the layout.
#
#   Parameters:
#
#       string - The string to break.
#
#   Returns:
#
#       The string with hidden breaks.
#
sub AddHiddenBreaks #(string)
    {
    my ($self, $string) = @_;

    # \.(?=.{5,}) instead of \. so file extensions don't get breaks.
    # :+ instead of :: because Mac paths are separated by a : and we want to get those too.

    $string =~ s/(\w(?:\.(?=.{5,})|:+|->|\\|\/))(\w)/$1 . '<span class=HB> <\/span>' . $2/ge;

    return $string;
    };

#
#   Function: FindFirstFile
#
#   A function that finds and returns the first file entry in the menu, or undef if none.
#
sub FindFirstFile
    {
    # Hidden parameter: arrayref
    # Used for recursion only.

    my ($self, $arrayref) = @_;

    if (!defined $arrayref)
        {  $arrayref = NaturalDocs::Menu->Content();  };

    foreach my $entry (@$arrayref)
        {
        if ($entry->Type() == ::MENU_FILE())
            {
            return $entry;
            }
        elsif ($entry->Type() == ::MENU_GROUP())
            {
            my $result = $self->FindFirstFile($entry->GroupContent());
            if (defined $result)
                {  return $result;  };
            };
        };

    return undef;
    };


#
#   Function: ExpandMenu
#
#   Determines which groups should be expanded.
#
#   Parameters:
#
#       sourceFile - The source <FileName> to use if you're looking for a source file.
#       indexType - The index <TopicType> to use if you're looking for an index.
#       selectionHierarchy - The <FileName> the menu is being built for.  Does not have to be on the menu itself.
#       rootLength - The length of the menu's root group, *not* including the contents of subgroups.
#
#   Returns:
#
#       An arrayref of all the group numbers that should be expanded.  At minimum, it will contain the numbers of the groups
#       present in <menuSelectionHierarchy>, though it may contain more.
#
sub ExpandMenu #(FileName sourceFile, TopicType indexType, NaturalDocs::Menu::Entry[] selectionHierarchy, int rootLength) -> int[] groupsToExpand
    {
    my ($self, $sourceFile, $indexType, $menuSelectionHierarchy, $rootLength) = @_;

    my $toExpand = [ ];


    # First expand everything in the selection hierarchy.

    my $length = $rootLength;

    foreach my $entry (@$menuSelectionHierarchy)
        {
        $length += $menuGroupLengths{$entry};
        push @$toExpand, $menuGroupNumbers{$entry};
        };


    # Now do multiple passes of group expansion as necessary.  We start from bottomIndex and expand outwards.  We stop going
    # in a direction if a group there is too long -- we do not skip over it and check later groups as well.  However, if one direction
    # stops, the other can keep going.

    my $pass = 1;
    my $hasSubGroups;

    while ($length < MENU_LENGTH_LIMIT)
        {
        my $content;
        my $topIndex;
        my $bottomIndex;


        if ($pass == 1)
            {
            # First pass, we expand the selection's siblings.

            if (scalar @$menuSelectionHierarchy)
                {  $content = $menuSelectionHierarchy->[0]->GroupContent();  }
            else
                {  $content = NaturalDocs::Menu->Content();  };

            $bottomIndex = 0;

            while ($bottomIndex < scalar @$content &&
                     !($content->[$bottomIndex]->Type() == ::MENU_FILE() &&
                       $content->[$bottomIndex]->Target() eq $sourceFile) &&
                     !($content->[$bottomIndex]->Type() != ::MENU_INDEX() &&
                       $content->[$bottomIndex]->Target() eq $indexType) )
                {  $bottomIndex++;  };

            if ($bottomIndex == scalar @$content)
                {  $bottomIndex = 0;  };
            $topIndex = $bottomIndex - 1;
            }

        elsif ($pass == 2)
            {
            # If the section we just expanded had no sub-groups, do another pass trying to expand the parent's sub-groups.  The
            # net effect is that groups won't collapse as much unnecessarily.  Someone can click on a file in a sub-group and the
            # groups in the parent will stay open.

            if (!$hasSubGroups && scalar @$menuSelectionHierarchy)
                {
                if (scalar @$menuSelectionHierarchy > 1)
                    {  $content = $menuSelectionHierarchy->[1]->GroupContent();  }
                else
                    {  $content = NaturalDocs::Menu->Content();  };

                $bottomIndex = 0;

                while ($bottomIndex < scalar @$content &&
                         $content->[$bottomIndex] != $menuSelectionHierarchy->[0])
                    {  $bottomIndex++;  };

                $topIndex = $bottomIndex - 1;
                $bottomIndex++;  # Increment past our own group.
                $hasSubGroups = undef;
                }
            else
                {  last;  };
            }

        # No more passes.
        else
            {  last;  };


        while ( ($topIndex >= 0 || $bottomIndex < scalar @$content) && $length < MENU_LENGTH_LIMIT)
            {
            # We do the bottom first.

            while ($bottomIndex < scalar @$content && $content->[$bottomIndex]->Type() != ::MENU_GROUP())
                {  $bottomIndex++;  };

            if ($bottomIndex < scalar @$content)
                {
                my $bottomEntry = $content->[$bottomIndex];
                $hasSubGroups = 1;

                if ($length + $menuGroupLengths{$bottomEntry} <= MENU_LENGTH_LIMIT)
                    {
                    $length += $menuGroupLengths{$bottomEntry};
                    push @$toExpand, $menuGroupNumbers{$bottomEntry};
                    $bottomIndex++;
                    }
                else
                    {  $bottomIndex = scalar @$content;  };
                };

            # Top next.

            while ($topIndex >= 0 && $content->[$topIndex]->Type() != ::MENU_GROUP())
                {  $topIndex--;  };

            if ($topIndex >= 0)
                {
                my $topEntry = $content->[$topIndex];
                $hasSubGroups = 1;

                if ($length + $menuGroupLengths{$topEntry} <= MENU_LENGTH_LIMIT)
                    {
                    $length += $menuGroupLengths{$topEntry};
                    push @$toExpand, $menuGroupNumbers{$topEntry};
                    $topIndex--;
                    }
                else
                    {  $topIndex = -1;  };
                };
            };


        $pass++;
        };

    return $toExpand;
    };


#
#   Function: GetMenuSelectionHierarchy
#
#   Finds the sequence of menu groups that contain the current selection.
#
#   Parameters:
#
#       sourceFile - The source <FileName> to use if you're looking for a source file.
#       indexType - The index <TopicType> to use if you're looking for an index.
#
#   Returns:
#
#       An arrayref of the <NaturalDocs::Menu::Entry> objects of each group surrounding the selected menu item.  First entry is the
#       group immediately encompassing it, and each subsequent entry works its way towards the outermost group.
#
sub GetMenuSelectionHierarchy #(FileName sourceFile, TopicType indexType) -> NaturalDocs::Menu::Entry[] selectionHierarchy
    {
    my ($self, $sourceFile, $indexType) = @_;

    my $hierarchy = [ ];

    $self->FindMenuSelection($sourceFile, $indexType, $hierarchy, NaturalDocs::Menu->Content());

    return $hierarchy;
    };


#
#   Function: FindMenuSelection
#
#   A recursive function that deterimes if it or any of its sub-groups has the menu selection.
#
#   Parameters:
#
#       sourceFile - The source <FileName> to use if you're looking for a source file.
#       indexType - The index <TopicType> to use if you're looking for an index.
#       hierarchyRef - A reference to the menu selection hierarchy.
#       entries - An arrayref of <NaturalDocs::Menu::Entries> to search.
#
#   Returns:
#
#       Whether this group or any of its subgroups had the selection.  If true, it will add any subgroups to the menu selection
#       hierarchy but not itself.  This prevents the topmost entry from being added.
#
sub FindMenuSelection #(FileName sourceFile, TopicType indexType, NaturalDocs::Menu::Entry[] hierarchyRef, NaturalDocs::Menu::Entry[] entries) -> bool hasSelection
    {
    my ($self, $sourceFile, $indexType, $hierarchyRef, $entries) = @_;

    foreach my $entry (@$entries)
        {
        if ($entry->Type() == ::MENU_GROUP())
            {
            # If the subgroup has the selection...
            if ( $self->FindMenuSelection($sourceFile, $indexType, $hierarchyRef, $entry->GroupContent()) )
                {
                push @$hierarchyRef, $entry;
                return 1;
                };
            }

        elsif ($entry->Type() == ::MENU_FILE())
            {
            if ($sourceFile eq $entry->Target())
                {  return 1;  };
            }

        elsif ($entry->Type() == ::MENU_INDEX())
            {
            if ($indexType eq $entry->Target)
                {  return 1;  };
            };
        };

    return 0;
    };


#
#   Function: ResetToolTips
#
#   Resets the <ToolTip Package Variables> for a new page.
#
#   Parameters:
#
#       samePage  - Set this flag if there's the possibility that the next batch of tooltips may be on the same page as the last.
#
sub ResetToolTips #(samePage)
    {
    my ($self, $samePage) = @_;

    if (!$samePage)
        {
        $tooltipLinkNumber = 1;
        $tooltipNumber = 1;
        };

    $tooltipHTML = undef;
    %tooltipSymbolsToNumbers = ( );
    };



1;
