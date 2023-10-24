#!/usr/bin/perl

use strict;
use warnings;
use IO::Handle;

# Enable auto-flush for STDOUT
STDOUT->autoflush(1);

# Set the content type to plain text
print "Content-Type: text/plain\r\n\r\n";

# Read and echo the POST data from stdin
while (<STDIN>) {
    print $_;
}

