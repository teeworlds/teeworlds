###############################################################################
#
#   Package: NaturalDocs::Project
#
###############################################################################
#
#   A package that manages information about the files in the source tree, as well as the list of files that have to be parsed
#   and built.
#
#   Usage and Dependencies:
#
#       - All the <Data File Functions> are available immediately, except for the status functions.
#
#       - <ReparseEverything()> and <RebuildEverything()> are available immediately, because they may need to be called
#         after <LoadConfigFileInfo()> but before <LoadSourceFileInfo()>.
#
#       - Prior to <LoadConfigFileInfo()>, <NaturalDocs::Settings> must be initialized.
#
#       - After <LoadConfigFileInfo()>, the status <Data File Functions> are available as well.
#
#       - Prior to <LoadSourceFileInfo()>, <NaturalDocs::Settings> and <NaturalDocs::Languages> must be initialized.
#
#       - After <LoadSourceFileInfo()>, the rest of the <Source File Functions> are available.
#
###############################################################################

# This file is part of Natural Docs, which is Copyright (C) 2003-2005 Greg Valure
# Natural Docs is licensed under the GPL

use NaturalDocs::Project::File;

use strict;
use integer;

package NaturalDocs::Project;


###############################################################################
# Group: Variables

#
#   handle: FH_FILEINFO
#
#   The file handle for the file information file, <FileInfo.nd>.
#


#
#   handle: FH_CONFIGFILEINFO
#
#   The file handle for the config file information file, <ConfigFileInfo.nd>.
#


#
#   hash: supportedFiles
#
#   A hash of all the supported files in the input directory.  The keys are the <FileNames>, and the values are
#   <NaturalDocs::Project::File> objects.
#
my %supportedFiles;

#
#   hash: filesToParse
#
#   An existence hash of all the <FileNames> that need to be parsed.
#
my %filesToParse;

#
#   hash: filesToBuild
#
#   An existence hash of all the <FileNames> that need to be built.
#
my %filesToBuild;

#
#   hash: filesToPurge
#
#   An existence hash of the <FileNames> that had Natural Docs content last time, but now either don't exist or no longer have
#   content.
#
my %filesToPurge;

#
#   hash: unbuiltFilesWithContent
#
#   An existence hash of all the <FileNames> that have Natural Docs content but are not part of <filesToBuild>.
#
my %unbuiltFilesWithContent;


# var: menuFileStatus
# The <FileStatus> of the project's menu file.
my $menuFileStatus;

# var: mainTopicsFileStatus
# The <FileStatus> of the project's main topics file.
my $mainTopicsFileStatus;

# var: userTopicsFileStatus
# The <FileStatus> of the project's user topics file.
my $userTopicsFileStatus;

# var: mainLanguagesFileStatus
# The <FileStatus> of the project's main languages file.
my $mainLanguagesFileStatus;

# var: userLanguagesFileStatus
# The <FileStatus> of the project's user languages file.
my $userLanguagesFileStatus;

# bool: reparseEverything
# Whether all the source files need to be reparsed.
my $reparseEverything;

# bool: rebuildEverything
# Whether all the source files need to be rebuilt.
my $rebuildEverything;

# hash: mostUsedLanguage
# The name of the most used language.  Doesn't include text files.
my $mostUsedLanguage;



###############################################################################
# Group: Files


#
#   File: FileInfo.nd
#
#   An index of the state of the files as of the last parse.  Used to determine if files were added, deleted, or changed.
#
#   Format:
#
#       The format is a text file.
#
#       > [VersionInt: app version]
#
#       The beginning of the file is the <VersionInt> it was generated with.
#
#       > [most used language name]
#
#       Next is the name of the most used language in the source tree.  Does not include text files.
#
#       Each following line is
#
#       > [file name] tab [last modification time] tab [has ND content (0 or 1)] tab [default menu title] \n
#
#   Revisions:
#
#       1.3:
#
#           - The line following the <VersionInt>, which was previously the last modification time of <Menu.txt>, was changed to
#             the name of the most used language.
#
#       1.16:
#
#           - File names are now absolute.  Prior to 1.16, they were relative to the input directory since only one was allowed.
#
#       1.14:
#
#           - The file was renamed from NaturalDocs.files to FileInfo.nd and moved into the Data subdirectory.
#
#       0.95:
#
#           - The file version was changed to match the program version.  Prior to 0.95, the version line was 1.  Test for "1" instead
#             of "1.0" to distinguish.
#


#
#   File: ConfigFileInfo.nd
#
#   An index of the state of the config files as of the last parse.
#
#   Format:
#
#       > [BINARY_FORMAT]
#       > [VersionInt: app version]
#
#       First is the standard <BINARY_FORMAT> <VersionInt> header.
#
#       > [UInt32: last modification time of menu]
#       > [UInt32: last modification of main topics file]
#       > [UInt32: last modification of user topics file]
#       > [UInt32: last modification of main languages file]
#       > [UInt32: last modification of user languages file]
#
#       Next are the last modification times of various configuration files as UInt32s in the standard Unix format.
#
#
#   Revisions:
#
#       1.3:
#
#           - The file was added to Natural Docs.  Previously the last modification of <Menu.txt> was stored in <FileInfo.nd>, and
#             <Topics.txt> and <Languages.txt> didn't exist.
#



###############################################################################
# Group: File Functions

#
#   Function: LoadSourceFileInfo
#
#   Loads the project file from disk and compares it against the files in the input directory.  Project is loaded from
#   <FileInfo.nd>.  New and changed files will be added to <FilesToParse()>, and if they have content,
#   <FilesToBuild()>.
#
#   Will call <NaturalDocs::Languages->OnMostUsedLanguageChange()> if <MostUsedLanguage()> changes.
#
#   Returns:
#
#       Returns whether the project was changed in any way.
#
sub LoadSourceFileInfo
    {
    my ($self) = @_;

    $self->GetAllSupportedFiles();
    NaturalDocs::Languages->OnMostUsedLanguageKnown();

    my $fileIsOkay;
    my $version;
    my $hasChanged;

    if (open(FH_FILEINFO, '<' . $self->FileInfoFile()))
        {
        # Check if the file is in the right format.
        $version = NaturalDocs::Version->FromTextFile(\*FH_FILEINFO);

        # The project file need to be rebuilt for 1.16.  The source files need to be reparsed and the output files rebuilt for 1.35.
        # We'll tolerate the difference between 1.16 and 1.3 in the loader.

        if ($version >= NaturalDocs::Version->FromString('1.16') && $version <= NaturalDocs::Settings->AppVersion())
            {
            $fileIsOkay = 1;

            if ($version < NaturalDocs::Version->FromString('1.35'))
                {
                $reparseEverything = 1;
                $rebuildEverything = 1;
                $hasChanged = 1;
                };
            }
        else
            {
            close(FH_FILEINFO);
            $hasChanged = 1;
            };
        };


    if ($fileIsOkay)
        {
        my %indexedFiles;


        my $line = <FH_FILEINFO>;
        ::XChomp(\$line);

        # Prior to 1.3 it was the last modification time of Menu.txt, which we ignore and treat as though the most used language
        # changed.  Prior to 1.32 the settings didn't transfer over correctly to Menu.txt so we need to behave that way again.
        if ($version < NaturalDocs::Version->FromString('1.32') || lc($mostUsedLanguage) ne lc($line))
            {
            $reparseEverything = 1;
            NaturalDocs::SymbolTable->RebuildAllIndexes();
            };


        # Parse the rest of the file.

        while ($line = <FH_FILEINFO>)
            {
            ::XChomp(\$line);
            my ($file, $modification, $hasContent, $menuTitle) = split(/\t/, $line, 4);

            # If the file no longer exists...
            if (!exists $supportedFiles{$file})
                {
                if ($hasContent)
                    {  $filesToPurge{$file} = 1;  };

                $hasChanged = 1;
                }

            # If the file still exists...
            else
                {
                $indexedFiles{$file} = 1;

                # If the file changed...
                if ($supportedFiles{$file}->LastModified() != $modification)
                    {
                    $supportedFiles{$file}->SetStatus(::FILE_CHANGED());
                    $filesToParse{$file} = 1;

                    # If the file loses its content, this will be removed by SetHasContent().
                    if ($hasContent)
                        {  $filesToBuild{$file} = 1;  };

                    $hasChanged = 1;
                    }

                # If the file has not changed...
                else
                    {
                    my $status;

                    if ($rebuildEverything && $hasContent)
                        {
                        $status = ::FILE_CHANGED();

                        # If the file loses its content, this will be removed by SetHasContent().
                        $filesToBuild{$file} = 1;
                        $hasChanged = 1;
                        }
                    else
                        {
                        $status = ::FILE_SAME();

                        if ($hasContent)
                            {  $unbuiltFilesWithContent{$file} = 1;  };
                        };

                    if ($reparseEverything)
                        {
                        $status = ::FILE_CHANGED();

                        $filesToParse{$file} = 1;
                        $hasChanged = 1;
                        };

                    $supportedFiles{$file}->SetStatus($status);
                    };

                $supportedFiles{$file}->SetHasContent($hasContent);
                $supportedFiles{$file}->SetDefaultMenuTitle($menuTitle);
                };
            };

        close(FH_FILEINFO);


        # Check for added files.

        if (scalar keys %supportedFiles > scalar keys %indexedFiles)
            {
            foreach my $file (keys %supportedFiles)
                {
                if (!exists $indexedFiles{$file})
                    {
                    $supportedFiles{$file}->SetStatus(::FILE_NEW());
                    $supportedFiles{$file}->SetDefaultMenuTitle($file);
                    $supportedFiles{$file}->SetHasContent(undef);
                    $filesToParse{$file} = 1;
                    # It will be added to filesToBuild if HasContent gets set to true when it's parsed.
                    $hasChanged = 1;
                    };
                };
            };
        }

    # If something's wrong with FileInfo.nd, everything is new.
    else
        {
        foreach my $file (keys %supportedFiles)
            {
            $supportedFiles{$file}->SetStatus(::FILE_NEW());
            $supportedFiles{$file}->SetDefaultMenuTitle($file);
            $supportedFiles{$file}->SetHasContent(undef);
            $filesToParse{$file} = 1;
            # It will be added to filesToBuild if HasContent gets set to true when it's parsed.
            };

        $hasChanged = 1;
        };


    # There are other side effects, so we need to call this.
    if ($rebuildEverything)
        {  $self->RebuildEverything();  };


    return $hasChanged;
    };


#
#   Function: SaveSourceFileInfo
#
#   Saves the source file info to disk.  Everything is saved in <FileInfo.nd>.
#
sub SaveSourceFileInfo
    {
    my ($self) = @_;

    open(FH_FILEINFO, '>' . $self->FileInfoFile())
        or die "Couldn't save project file " . $self->FileInfoFile() . "\n";

    NaturalDocs::Version->ToTextFile(\*FH_FILEINFO, NaturalDocs::Settings->AppVersion());

    print FH_FILEINFO $mostUsedLanguage . "\n";

    while (my ($fileName, $file) = each %supportedFiles)
        {
        print FH_FILEINFO $fileName . "\t"
                              . $file->LastModified() . "\t"
                              . ($file->HasContent() || '0') . "\t"
                              . $file->DefaultMenuTitle() . "\n";
        };

    close(FH_FILEINFO);
    };


#
#   Function: LoadConfigFileInfo
#
#   Loads the config file info to disk.
#
sub LoadConfigFileInfo
    {
    my ($self) = @_;

    my $fileIsOkay;
    my $version;
    my $fileName = NaturalDocs::Project->ConfigFileInfoFile();

    if (open(FH_CONFIGFILEINFO, '<' . $fileName))
        {
        # See if it's binary.
        binmode(FH_CONFIGFILEINFO);

        my $firstChar;
        read(FH_CONFIGFILEINFO, $firstChar, 1);

        if ($firstChar == ::BINARY_FORMAT())
            {
            $version = NaturalDocs::Version->FromBinaryFile(\*FH_CONFIGFILEINFO);

            # It hasn't changed since being introduced.

            if ($version <= NaturalDocs::Settings->AppVersion())
                {  $fileIsOkay = 1;  }
            else
                {  close(FH_CONFIGFILEINFO);  };
            }

        else # it's not in binary
            {  close(FH_CONFIGFILEINFO);  };
        };

    my @configFiles = ( $self->MenuFile(), \$menuFileStatus,
                                 $self->MainTopicsFile(), \$mainTopicsFileStatus,
                                 $self->UserTopicsFile(), \$userTopicsFileStatus,
                                 $self->MainLanguagesFile(), \$mainLanguagesFileStatus,
                                 $self->UserLanguagesFile(), \$userLanguagesFileStatus );

    if ($fileIsOkay)
        {
        my $raw;

        read(FH_CONFIGFILEINFO, $raw, 20);
        my @configFileDates = unpack('NNNNN', $raw);

        while (scalar @configFiles)
            {
            my $file = shift @configFiles;
            my $fileStatus = shift @configFiles;
            my $fileDate = shift @configFileDates;

            if (-e $file)
                {
                if ($fileDate == (stat($file))[9])
                    {  $$fileStatus = ::FILE_SAME();  }
                else
                    {  $$fileStatus = ::FILE_CHANGED();  };
                }
            else
                {  $$fileStatus = ::FILE_DOESNTEXIST();  };
            };

        close(FH_CONFIGFILEINFO);
        }
    else
        {
        while (scalar @configFiles)
            {
            my $file = shift @configFiles;
            my $fileStatus = shift @configFiles;

            if (-e $file)
                {  $$fileStatus = ::FILE_CHANGED();  }
            else
                {  $$fileStatus = ::FILE_DOESNTEXIST();  };
            };
        };

    if ($menuFileStatus == ::FILE_SAME() && $rebuildEverything)
        {  $menuFileStatus = ::FILE_CHANGED();  };
    };


#
#   Function: SaveConfigFileInfo
#
#   Saves the config file info to disk.  You *must* save all other config files first, such as <Menu.txt> and <Topics.txt>.
#
sub SaveConfigFileInfo
    {
    my ($self) = @_;

    open (FH_CONFIGFILEINFO, '>' . NaturalDocs::Project->ConfigFileInfoFile())
        or die "Couldn't save " . NaturalDocs::Project->ConfigFileInfoFile() . ".\n";

    binmode(FH_CONFIGFILEINFO);

    print FH_CONFIGFILEINFO '' . ::BINARY_FORMAT();

    NaturalDocs::Version->ToBinaryFile(\*FH_CONFIGFILEINFO, NaturalDocs::Settings->AppVersion());

    print FH_CONFIGFILEINFO pack('NNNNN', (stat($self->MenuFile()))[9],
                                                                (stat($self->MainTopicsFile()))[9],
                                                                (stat($self->UserTopicsFile()))[9],
                                                                (stat($self->MainLanguagesFile()))[9],
                                                                (stat($self->UserLanguagesFile()))[9] );

    close(FH_CONFIGFILEINFO);
    };


#
#   Function: MigrateOldFiles
#
#   If the project uses the old file names used prior to 1.14, it converts them to the new file names.
#
sub MigrateOldFiles
    {
    my ($self) = @_;

    my $projectDirectory = NaturalDocs::Settings->ProjectDirectory();

    # We use the menu file as a test to see if we're using the new format.
    if (-e NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs_Menu.txt'))
        {
        # The Data subdirectory would have been created by NaturalDocs::Settings.

        rename( NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs_Menu.txt'), $self->MenuFile() );

        if (-e NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.sym'))
            {  rename( NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.sym'), $self->SymbolTableFile() );  };

        if (-e NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.files'))
            {  rename( NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.files'), $self->FileInfoFile() );  };

        if (-e NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.m'))
            {  rename( NaturalDocs::File->JoinPaths($projectDirectory, 'NaturalDocs.m'), $self->PreviousMenuStateFile() );  };
        };
    };



###############################################################################
# Group: Data File Functions


# Function: FileInfoFile
# Returns the full path to the file information file.
sub FileInfoFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'FileInfo.nd' );  };

# Function: ConfigFileInfoFile
# Returns the full path to the config file information file.
sub ConfigFileInfoFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'ConfigFileInfo.nd' );  };

# Function: SymbolTableFile
# Returns the full path to the symbol table's data file.
sub SymbolTableFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'SymbolTable.nd' );  };

# Function: ClassHierarchyFile
# Returns the full path to the class hierarchy's data file.
sub ClassHierarchyFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'ClassHierarchy.nd' );  };

# Function: MenuFile
# Returns the full path to the project's menu file.
sub MenuFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), 'Menu.txt' );  };

# Function: MenuFileStatus
# Returns the <FileStatus> of the project's menu file.
sub MenuFileStatus
    {  return $menuFileStatus;  };

# Function: MainTopicsFile
# Returns the full path to the main topics file.
sub MainTopicsFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ConfigDirectory(), 'Topics.txt' );  };

# Function: MainTopicsFileStatus
# Returns the <FileStatus> of the project's main topics file.
sub MainTopicsFileStatus
    {  return $mainTopicsFileStatus;  };

# Function: UserTopicsFile
# Returns the full path to the user's topics file.
sub UserTopicsFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), 'Topics.txt' );  };

# Function: UserTopicsFileStatus
# Returns the <FileStatus> of the project's user topics file.
sub UserTopicsFileStatus
    {  return $userTopicsFileStatus;  };

# Function: MainLanguagesFile
# Returns the full path to the main languages file.
sub MainLanguagesFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ConfigDirectory(), 'Languages.txt' );  };

# Function: MainLanguagesFileStatus
# Returns the <FileStatus> of the project's main languages file.
sub MainLanguagesFileStatus
    {  return $mainLanguagesFileStatus;  };

# Function: UserLanguagesFile
# Returns the full path to the user's languages file.
sub UserLanguagesFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), 'Languages.txt' );  };

# Function: UserLanguagesFileStatus
# Returns the <FileStatus> of the project's user languages file.
sub UserLanguagesFileStatus
    {  return $userLanguagesFileStatus;  };

# Function: SettingsFile
# Returns the full path to the project's settings file.
sub SettingsFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), 'Settings.txt' );  };

# Function: PreviousSettingsFile
# Returns the full path to the project's previous settings file.
sub PreviousSettingsFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'PreviousSettings.nd' );  };

# Function: PreviousMenuStateFile
# Returns the full path to the project's previous menu state file.
sub PreviousMenuStateFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDataDirectory(), 'PreviousMenuState.nd' );  };

# Function: MenuBackupFile
# Returns the full path to the project's menu backup file, which is used to save the original menu in some situations.
sub MenuBackupFile
    {  return NaturalDocs::File->JoinPaths( NaturalDocs::Settings->ProjectDirectory(), 'Menu_Backup.txt' );  };



###############################################################################
# Group: Source File Functions


# Function: FilesToParse
# Returns an existence hashref of the <FileNames> to parse.  This is not a copy of the data, so don't change it.
sub FilesToParse
    {  return \%filesToParse;  };

# Function: FilesToBuild
# Returns an existence hashref of the <FileNames> to build.  This is not a copy of the data, so don't change it.
sub FilesToBuild
    {  return \%filesToBuild;  };

# Function: FilesToPurge
# Returns an existence hashref of the <FileNames> that had content last time, but now either don't anymore or were deleted.
# This is not a copy of the data, so don't change it.
sub FilesToPurge
    {  return \%filesToPurge;  };

#
#   Function: RebuildFile
#
#   Adds the file to the list of files to build.  This function will automatically filter out files that don't have Natural Docs content and
#   files that are part of <FilesToPurge()>.  If this gets called on a file and that file later gets Natural Docs content, it will be added.
#
#   Parameters:
#
#       file - The <FileName> to build or rebuild.
#
sub RebuildFile #(file)
    {
    my ($self, $file) = @_;

    # We don't want to add it to the build list if it doesn't exist, doesn't have Natural Docs content, or it's going to be purged.
    # If it wasn't parsed yet and will later be found to have ND content, it will be added by SetHasContent().
    if (exists $supportedFiles{$file} && !exists $filesToPurge{$file} && $supportedFiles{$file}->HasContent())
        {
        $filesToBuild{$file} = 1;

        if (exists $unbuiltFilesWithContent{$file})
            {  delete $unbuiltFilesWithContent{$file};  };
        };
    };


#
#   Function: ReparseEverything
#
#   Adds all supported files to the list of files to parse.  This does not necessarily mean these files are going to be rebuilt.
#
sub ReparseEverything
    {
    my ($self) = @_;

    if (!$reparseEverything)
        {
        foreach my $file (keys %supportedFiles)
            {
            $filesToParse{$file} = 1;
            };

        $reparseEverything = 1;
        };
    };


#
#   Function: RebuildEverything
#
#   Adds all supported files to the list of files to build.  This does not necessarily mean these files are going to be reparsed.
#
sub RebuildEverything
    {
    my ($self) = @_;

    foreach my $file (keys %unbuiltFilesWithContent)
        {
        $filesToBuild{$file} = 1;
        };

    %unbuiltFilesWithContent = ( );
    $rebuildEverything = 1;

    NaturalDocs::SymbolTable->RebuildAllIndexes();

    if ($menuFileStatus == ::FILE_SAME())
        {  $menuFileStatus = ::FILE_CHANGED();  };
    };


# Function: UnbuiltFilesWithContent
# Returns an existence hashref of the <FileNames> that have Natural Docs content but are not part of <FilesToBuild()>.  This is
# not a copy of the data so don't change it.
sub UnbuiltFilesWithContent
    {  return \%unbuiltFilesWithContent;  };

# Function: FilesWithContent
# Returns and existence hashref of the <FileNames> that have Natural Docs content.
sub FilesWithContent
    {
    # Don't keep this one internally, but there's an easy way to make it.
    return { %filesToBuild, %unbuiltFilesWithContent };
    };


#
#   Function: HasContent
#
#   Returns whether the <FileName> contains Natural Docs content.
#
sub HasContent #(file)
    {
    my ($self, $file) = @_;

    if (exists $supportedFiles{$file})
        {  return $supportedFiles{$file}->HasContent();  }
    else
        {  return undef;  };
    };


#
#   Function: SetHasContent
#
#   Sets whether the <FileName> has Natural Docs content or not.
#
sub SetHasContent #(file, hasContent)
    {
    my ($self, $file, $hasContent) = @_;

    if (exists $supportedFiles{$file} && $supportedFiles{$file}->HasContent() != $hasContent)
        {
        # If the file now has content...
        if ($hasContent)
            {
            $filesToBuild{$file} = 1;
            }

        # If the file's content has been removed...
        else
            {
            delete $filesToBuild{$file};  # may not be there
            $filesToPurge{$file} = 1;
            };

        $supportedFiles{$file}->SetHasContent($hasContent);
        };
    };


#
#   Function: StatusOf
#
#   Returns the <FileStatus> of the passed <FileName>.
#
sub StatusOf #(file)
    {
    my ($self, $file) = @_;

    if (exists $supportedFiles{$file})
        {  return $supportedFiles{$file}->Status();  }
    else
        {  return ::FILE_DOESNTEXIST();  };
    };


#
#   Function: DefaultMenuTitleOf
#
#   Returns the default menu title of the <FileName>.  If one isn't specified, it returns the <FileName>.
#
sub DefaultMenuTitleOf #(file)
    {
    my ($self, $file) = @_;

    if (exists $supportedFiles{$file})
        {  return $supportedFiles{$file}->DefaultMenuTitle();  }
    else
        {  return $file;  };
    };


#
#   Function: SetDefaultMenuTitle
#
#   Sets the <FileName's> default menu title.
#
sub SetDefaultMenuTitle #(file, menuTitle)
    {
    my ($self, $file, $menuTitle) = @_;

    if (exists $supportedFiles{$file} && $supportedFiles{$file}->DefaultMenuTitle() ne $menuTitle)
        {
        $supportedFiles{$file}->SetDefaultMenuTitle($menuTitle);
        NaturalDocs::Menu->OnDefaultTitleChange($file);
        };
    };


#
#   Function: MostUsedLanguage
#
#   Returns the name of the most used language in the source trees.  Does not include text files.
#
sub MostUsedLanguage
    {  return $mostUsedLanguage;  };



###############################################################################
# Group: Support Functions

#
#   Function: GetAllSupportedFiles
#
#   Gets all the supported files in the passed directory and its subdirectories and puts them into <supportedFiles>.  The only
#   attribute that will be set is <NaturalDocs::Project::File->LastModified()>.  Also sets <mostUsedLanguage>.
#
sub GetAllSupportedFiles
    {
    my ($self) = @_;

    my @directories = @{NaturalDocs::Settings->InputDirectories()};

    # Keys are language names, values are counts.
    my %languageCounts;


    # Make an existence hash of excluded directories.

    my %excludedDirectories;
    my $excludedDirectoryArrayRef = NaturalDocs::Settings->ExcludedInputDirectories();

    foreach my $excludedDirectory (@$excludedDirectoryArrayRef)
        {
        if (NaturalDocs::File->IsCaseSensitive())
            {  $excludedDirectories{$excludedDirectory} = 1;  }
        else
            {  $excludedDirectories{lc($excludedDirectory)} = 1;  };
        };


    while (scalar @directories)
        {
        my $directory = pop @directories;

        opendir DIRECTORYHANDLE, $directory;
        my @entries = readdir DIRECTORYHANDLE;
        closedir DIRECTORYHANDLE;

        @entries = NaturalDocs::File->NoUpwards(@entries);

        foreach my $entry (@entries)
            {
            my $fullEntry = NaturalDocs::File->JoinPaths($directory, $entry);

            # If an entry is a directory, recurse.
            if (-d $fullEntry)
                {
                # Join again with the noFile flag set in case the platform handles them differently.
                $fullEntry = NaturalDocs::File->JoinPaths($directory, $entry, 1);

                if (NaturalDocs::File->IsCaseSensitive())
                    {
                    if (!exists $excludedDirectories{$fullEntry})
                        {  push @directories, $fullEntry;  };
                    }
                else
                    {
                    if (!exists $excludedDirectories{lc($fullEntry)})
                        {  push @directories, $fullEntry;  };
                    };
                }

            # Otherwise add it if it's a supported extension.
            else
                {
                if (my $language = NaturalDocs::Languages->LanguageOf($fullEntry))
                    {
                    $supportedFiles{$fullEntry} = NaturalDocs::Project::File->New(undef, (stat($fullEntry))[9], undef, undef);
                    $languageCounts{$language->Name()}++;
                    };
                };
            };
        };


    my $topCount = 0;

    while (my ($language, $count) = each %languageCounts)
        {
        if ($count > $topCount && $language ne 'Text File')
            {
            $topCount = $count;
            $mostUsedLanguage = $language;
            };
        };
    };


1;
