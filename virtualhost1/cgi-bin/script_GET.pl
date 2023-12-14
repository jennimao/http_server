#!/usr/bin/perl

use strict;
use warnings;
use IO::Handle;


# Send the Content-Type header to indicate that it's an HTML response
print "Content-Type: text/html\n\n";

# HTML response content
print "<html><head><title>CGI Perl Script</title></head>";
print "<body>";
print "<h1>Hello from CGI Perl Script</h1>";
print "<p>This is a simple CGI Perl script responding to a GET request.</p>";
print "</body></html>";

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

