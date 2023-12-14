#!/usr/bin/perl

use strict;
use warnings;
use IO::Handle;

# Set the content type to plain text
# print "Content-Type: text/plain\r\n\r\n";

# Read and echo the POST data from stdin
while (<STDIN>) {
    print $_;
}
print "\n\n";
print "Environment variables:\n";

# Print specific environment variables
print "QUERY_STRING: $ENV{'QUERY_STRING'}\n" if defined $ENV{'QUERY_STRING'};
print "REQUEST_METHOD: $ENV{'REQUEST_METHOD'}\n" if defined $ENV{'REQUEST_METHOD'};

# Print all REMOTE_* variables
foreach my $key (keys %ENV) {
    if ($key =~ /^REMOTE_/) {
        print "$key: $ENV{$key}\n";
    }
}

# Print all SERVER_* variables
foreach my $key (keys %ENV) {
    if ($key =~ /^SERVER_/) {
        print "$key: $ENV{$key}\n";
    }
} 