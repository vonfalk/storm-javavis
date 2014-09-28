#pragma once

#!/bin/perl -w

use Switch;

$num_args = $#ARGV + 1;
if ($num_args < 3) {
    print "Add:    -a project.vcproj file1...\n";
    print "Delete: -d project.vcproj file1...\n";
    print "Rename: -r project.vcproj from to\n";
    exit;
}

$mode = $ARGV[0];
$iFile = $ARGV[1];

@toInsert = ();
@toDelete = ();
$renameFrom = "";
$renameTo = "";

if ($mode eq "-a") {
    @toInsert = @ARGV;
    splice(@toInsert, 0, 2, ());
} elsif ($mode eq "-d") {
    @toDelete = @ARGV;
    splice(@toDelete, 0, 2, ());
} elsif ($mode eq "-r") {
    $renameFrom = $ARGV[2];
    $renameTo = $ARGV[3];
}

sub file_eq {
    my ($a, $b) = @_;

    $a =~ s/\\/\//g;
    $b =~ s/\\/\//g;

    $a =~ s/^.\///g;
    $b =~ s/^.\///g;

    return lc($a) eq lc($b);
}

sub striplist {
    my ($fn) = @_;

    for my $index (reverse 0..$#toInsert) {
	if (file_eq($toInsert[$index], $fn)) {
	    splice(@toInsert, $index, 1, ());
	}
    }
}

sub putpage {
    my ($path, $middle) = @_;

    if (file_eq($path, $renameFrom)) {
    	$path = $renameTo;
    }

    foreach (@toDelete) {
	if (file_eq($path, $_)) {
	    return;
	}
    }

    if ($path =~ m/^\.[\/\\]/) {
    } else {
	$path = "./".$path;
    }

    $path =~ s/\//\\/g;

    print "\t\t\t<File\r\n";
    print "\t\t\t\tRelativePath=\"${path}\"\r\n";
    print "\t\t\t\t>\r\n";
    print $middle;
    print "\t\t\t</File>\r\n";
}

sub putpages {
    my (@arr) = @_;

    foreach (@arr) {
	putpage($_);
    }
}

my $filepart = 0;
my $filename = "";
my $middle = "";

sub parsetag {
    my ($line) = @_;

    my $toThree = 0;

    if ($line =~ m/<File[\n\r\t ]/) {
	$filepart = 1;
    } elsif ($line =~ m/RelativePath="([^"]*)"/) {
	if ($filepart == 1) {
	    $filepart = 2;
	    $filename = $1;
	}
    } elsif ($line =~ m/<\/File>/) {
	if ($filepart == 3) {
	    $filepart = 4;
	}
    } elsif ($line =~ m/\w*>\w*/) {
	if ($filepart == 2) {
	    $filepart = 3;
	    $middle = "";
	    $toThree = 1;
	}
    }

    if ($filepart == 3) {
	if (!$toThree) {
	    $middle = $middle.$line;
	}
    }

    if ($filepart == 0) {
	print $line;
    }

    #print "Line ${filepart}: ${line}";

    return $filepart == 4;
}

sub at_end {
    my ($line) = @_;
    return $line =~ m/\w*<\/Filter>\w*/;
}

open FILE, "<", $iFile or die $!;
open STDOUT, ">", $iFile.".tmp" or die $!;

my $section = "";
my $output = 1;

while ($line = <FILE>) {
    switch ($section) {
	case "" {
	    $output = 1;
	    if ($line =~ m/\w*<Filter\w*/) {
		$section = "filter";
	    }
	}
	case "filter" {
	    $output = 1;
	    if ($line =~ m/\w*Name="([^"]*)"/) {
		$section = $1;
		$filepart = 0;
	    }
	}
	case "Header Files" {
	    $output = 0;
	    if (at_end($line)) {
		@r = grep(m/.*h/, @toInsert);
		putpages(@r);
		$section = "";
		$output = 1;
	    } elsif (parsetag($line)) {
		striplist($filename);
		putpage($filename, $middle);
	    }
	}
	case "Source Files" {
	    $output = 0;
	    if (at_end($line)) {
		@r = grep(m/.*cpp/, @toInsert);
		putpages(@r);
		$section = "";
		$output = 1;
	    } elsif (parsetag($line)) {
		striplist($filename);
		putpage($filename, $middle);
	    }
	}
    }

    if ($output) {
	print $line;
    }
}

close(FILE);
close(STDOUT);

unlink $iFile;
rename $iFile.".tmp", $iFile;

