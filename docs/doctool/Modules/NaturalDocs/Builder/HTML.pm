###############################################################################
#
#   Package: NaturalDocs::Builder::HTML
#
###############################################################################
#
#   A package that generates output in HTML.
#
#   All functions are called with Package->Function() notation.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL


use strict;
use integer;

package NaturalDocs::Builder::HTML;

use base 'NaturalDocs::Builder::HTMLBase';


###############################################################################
# Group: Implemented Interface Functions


#
#   Function: INIT
#
#   Registers the package with <NaturalDocs::Builder>.
#
sub INIT
    {
    NaturalDocs::Builder->Add(__PACKAGE__);
    };


#
#   Function: CommandLineOption
#
#   Returns the option to follow -o to use this package.  In this case, "html".
#
sub CommandLineOption
    {
    return 'HTML';
    };


#
#   Function: BuildFile
#
#   Builds the output file from the parsed source file.
#
#   Parameters:
#
#       sourcefile       - The <FileName> of the source file.
#       parsedFile      - An arrayref of the source file as <NaturalDocs::Parser::ParsedTopic> objects.
#
sub BuildFile #(sourceFile, parsedFile)
    {
    my ($self, $sourceFile, $parsedFile) = @_;

    my $outputFile = $self->OutputFileOf($sourceFile);


    # 99.99% of the time the output directory will already exist, so this will actually be more efficient.  It only won't exist
    # if a new file was added in a new subdirectory and this is the first time that file was ever parsed.
    if (!open(OUTPUTFILEHANDLE, '>' . $outputFile))
        {
        NaturalDocs::File->CreatePath( NaturalDocs::File->NoFileName($outputFile) );

        open(OUTPUTFILEHANDLE, '>' . $outputFile)
            or die "Couldn't create output file " . $outputFile . "\n";
        };

    print OUTPUTFILEHANDLE


        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" '
            . '"http://www.w3.org/TR/REC-html40/strict.dtd">' . "\n\n"

        . '<html><head>'

            . (NaturalDocs::Settings->CharSet() ?
                '<meta http-equiv="Content-Type" content="text/html; charset=' . NaturalDocs::Settings->CharSet() . '">' : '')

            . '<title>'
                . $self->BuildTitle($sourceFile)
            . '</title>'

            . '<link rel="stylesheet" type="text/css" href="' . $self->MakeRelativeURL($outputFile, $self->MainCSSFile(), 1) . '">'

            . '<script language=JavaScript src="' . $self->MakeRelativeURL($outputFile, $self->MainJavaScriptFile(), 1) . '"></script>'

        . '</head><body class=UnframedPage onLoad="NDOnLoad()">'
            . $self->OpeningBrowserStyles()

        . $self->StandardComments()

        # I originally had this part done in CSS, but there were too many problems.  Back to good old HTML tables.
        . '<table border=0 cellspacing=0 cellpadding=0 width=100%><tr>'

            . '<td class=MenuSection valign=top>'

                . $self->BuildMenu($sourceFile, undef, undef)

            . '</td>' . "\n\n"

            . '<td class=ContentSection valign=top>'

                . $self->BuildContent($sourceFile, $parsedFile)

            . '</td>' . "\n\n"

        . '</tr></table>'

        . '<div class=Footer>'
            . $self->BuildFooter()
        . '</div>'

        . $self->BuildToolTips()

            . $self->ClosingBrowserStyles()
        . '</body></html>';


    close(OUTPUTFILEHANDLE);
    };


#
#   Function: BuildIndex
#
#   Builds an index for the passed type.
#
#   Parameters:
#
#       type  - The <TopicType> to limit the index to, or undef if none.
#
sub BuildIndex #(type)
    {
    my ($self, $type) = @_;

    my $indexTitle = $self->IndexTitleOf($type);
    my $indexFile = $self->IndexFileOf($type);

    my $startPage =

        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" '
            . '"http://www.w3.org/TR/REC-html40/strict.dtd">' . "\n\n"

        . '<html><head>'

            . (NaturalDocs::Settings->CharSet() ?
                '<meta http-equiv="Content-Type" content="text/html; charset=' . NaturalDocs::Settings->CharSet() . '">' : '')

            . '<title>'
                . $indexTitle;

                if (defined NaturalDocs::Menu->Title())
                    {  $startPage .= ' - ' . $self->StringToHTML(NaturalDocs::Menu->Title());  };

            $startPage .=
            '</title>'

            . '<link rel="stylesheet" type="text/css" href="' . $self->MakeRelativeURL($indexFile, $self->MainCSSFile(), 1) . '">'

            . '<script language=JavaScript src="' . $self->MakeRelativeURL($indexFile, $self->MainJavaScriptFile(), 1) . '"></script>'

        . '</head><body class=UnframedPage onLoad="NDOnLoad()">'
            . $self->OpeningBrowserStyles()

        . $self->StandardComments()

        # I originally had this part done in CSS, but there were too many problems.  Back to good old HTML tables.
        . '<table border=0 cellspacing=0 cellpadding=0 width=100%><tr>'

            . '<td class=MenuSection valign=top>'

                . $self->BuildMenu(undef, $type, undef)

            . '</td>'

            . '<td class=IndexSection valign=top>'
                . '<div class=IPageTitle>'
                    . $indexTitle
                . '</div>';


    my $endPage =
            '</td>'

        . '</tr></table>'

        . '<div class=Footer>'
            . $self->BuildFooter()
        . '</div>'

            . $self->ClosingBrowserStyles()
        . '</body></html>';


    my $pageCount = $self->BuildIndexPages($type, NaturalDocs::SymbolTable->Index($type), $startPage, $endPage);
    $self->PurgeIndexFiles($type, $pageCount + 1);
    };


#
#   Function: UpdateMenu
#
#   Updates the menu in all the output files that weren't rebuilt.  Also generates index.html.
#
sub UpdateMenu
    {
    my $self = shift;


    # Update the menu on unbuilt files.

    my $filesToUpdate = NaturalDocs::Project->UnbuiltFilesWithContent();

    foreach my $sourceFile (keys %$filesToUpdate)
        {
        $self->UpdateFile($sourceFile);
        };


    # Update the menu on unchanged index files.

    my $indexes = NaturalDocs::Menu->Indexes();

    foreach my $index (keys %$indexes)
        {
        if (!NaturalDocs::SymbolTable->IndexChanged($index))
            {
            $self->UpdateIndex($index);
            };
        };


    # Update index.html

    my $firstMenuEntry = $self->FindFirstFile();
    my $indexFile = NaturalDocs::File->JoinPaths( NaturalDocs::Settings->OutputDirectoryOf($self), 'index.html' );

    # We have to check because it's possible that there may be no files with Natural Docs content and thus no files on the menu.
    if (defined $firstMenuEntry)
        {
        open(INDEXFILEHANDLE, '>' . $indexFile)
            or die "Couldn't create output file " . $indexFile . ".\n";

        print INDEXFILEHANDLE
        '<html><head>'
             . '<meta http-equiv="Refresh" CONTENT="0; URL='
                 . $self->MakeRelativeURL( NaturalDocs::File->JoinPaths( NaturalDocs::Settings->OutputDirectoryOf($self), 'index.html'),
                                                        $self->OutputFileOf($firstMenuEntry->Target()), 1 ) . '">'
        . '</head></html>';

        close INDEXFILEHANDLE;
        }

    elsif (-e $indexFile)
        {
        unlink($indexFile);
        };
    };



###############################################################################
# Group: Support Functions


#
#   Function: UpdateFile
#
#   Updates an output file.  Replaces the menu, HTML title, and footer.  It opens the output file, makes the changes, and saves it
#   back to disk, which is much quicker than rebuilding the file from scratch if these were the only things that changed.
#
#   Parameters:
#
#       sourceFile - The source <FileName>.
#
sub UpdateFile #(sourceFile)
    {
    my ($self, $sourceFile) = @_;

    my $outputFile = $self->OutputFileOf($sourceFile);

    if (open(OUTPUTFILEHANDLE, '<' . $outputFile))
        {
        my $content;

        read(OUTPUTFILEHANDLE, $content, -s OUTPUTFILEHANDLE);
        close(OUTPUTFILEHANDLE);


        $content =~ s{<title>[^<]*<\/title>}{'<title>' . $self->BuildTitle($sourceFile) . '</title>'}e;

        $content =~ s/<!--START_ND_MENU-->.*?<!--END_ND_MENU-->/$self->BuildMenu($sourceFile, undef, undef)/es;

        $content =~ s/<!--START_ND_FOOTER-->.*?<!--END_ND_FOOTER-->/$self->BuildFooter()/e;


        open(OUTPUTFILEHANDLE, '>' . $outputFile);
        print OUTPUTFILEHANDLE $content;
        close(OUTPUTFILEHANDLE);
        };
    };


#
#   Function: UpdateIndex
#
#   Updates an index's output file.  Replaces the menu and footer.  It opens the output file, makes the changes, and saves it
#   back to disk, which is much quicker than rebuilding the file from scratch if these were the only things that changed.
#
#   Parameters:
#
#       type - The index <TopicType>, or undef if none.
#
sub UpdateIndex #(type)
    {
    my ($self, $type) = @_;

    my $page = 1;

    my $outputFile = $self->IndexFileOf($type, $page);

    my $newMenu = $self->BuildMenu(undef, $type, undef);
    my $newFooter = $self->BuildFooter();

    while (-e $outputFile)
        {
        open(OUTPUTFILEHANDLE, '<' . $outputFile)
            or die "Couldn't open output file " . $outputFile . ".\n";

        my $content;

        read(OUTPUTFILEHANDLE, $content, -s OUTPUTFILEHANDLE);
        close(OUTPUTFILEHANDLE);


        $content =~ s/<!--START_ND_MENU-->.*?<!--END_ND_MENU-->/$newMenu/es;

        $content =~ s/<div class=Footer>.*<\/div>/"<div class=Footer>" . $newFooter . "<\/div>"/e;


        open(OUTPUTFILEHANDLE, '>' . $outputFile)
            or die "Couldn't save output file " . $outputFile . ".\n";

        print OUTPUTFILEHANDLE $content;
        close(OUTPUTFILEHANDLE);

        $page++;
        $outputFile = $self->IndexFileOf($type, $page);
        };
    };


1;
