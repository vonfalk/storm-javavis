#pragma once

#!/bin/perl -w

use Switch;

$num_args = $#ARGV + 1;
if ($num_args < 2) {
    print "\nUsage: project.vcproj, file1...\n";
    exit;
}

$iFile = $ARGV[0];
@toInsert = @ARGV;
splice(@toInsert, 0, 1, ());

sub file_eq {
    my ($a, $b) = @_;

    $a =~ s/\\/\//g;
    $b =~ s/\\/\//g;

    $a =~ s/^.\///g;
    $b =~ s/^.\///g;

    return $a eq $b;
}

sub striplist {
    my ($fn) = @_;

    for my $index (reverse 0..$#toInsert) {
	if (file_eq($toInsert[$index], $fn)) {
	    splice(@toInsert, $index, 1, ());
	}
    }
}

sub putpages {
    my (@arr) = @_;

    foreach (@arr) {
	my $path = $_;
	if ($path =~ m/^\n.\//) {
	} else {
	    $path = "./".$path;
	}

	$path =~ s/\//\\/g;

	print OUT "\t\t\t<File\r\n";
	print OUT "\t\t\t\tRelativePath=\"${path}\"\r\n";
	print OUT "\t\t\t\t>\r\n";
	print OUT "\t\t\t</File>\r\n";
    }
}

open FILE, "<", $iFile or die $!;
open OUT, ">", $iFile.".tmp" or die $!;

my $section = "";

while ($line = <FILE>) {
    switch ($section) {
	case "" {
	    if ($line =~ m/\w*<Filter\w*/) {
		$section = "filter";
	    }
	}
	case "filter" {
	    if ($line =~ m/\w*Name="([^"]*)"/) {
		$section = $1;
	    }
	}
	case "Header Files" {
	    if ($line =~ m/\w*RelativePath="([^"]*)"\w*/) {
		striplist($1);
	    } elsif ($line =~ m/\w*<\/Filter>\w*/) {
		@r = grep(m/.*h/, @toInsert);
		putpages(@r);
		$section = "";
	    }
	}
	case "Source Files" {
	    if ($line =~ m/\w*RelativePath="([^"]*)"\w*/) {
		striplist($1);
	    } elsif ($line =~ m/\w*<\/Filter>\w*/) {
		@r = grep(m/.*cpp/, @toInsert);
		putpages(@r);
		$section = "";
	    }
	}
    }

    print OUT $line;
}

close(FILE);
close(OUT);

unlink $iFile;
rename $iFile.".tmp", $iFile;

