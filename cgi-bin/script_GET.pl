#!/usr/bin/perl

use strict;
use warnings;
use IO::Handle;

# Enable auto-flush for STDOUT
STDOUT->autoflush(1);

# Send the Content-Type header to indicate that it's an HTML response
print "Content-Type: text/html\n\n";

# HTML response content
print "<html><head><title>CGI Perl Script</title></head>";
print "<body>";
print "<h1>Hello from CGI Perl Script</h1>";
print "<p>This is a simple CGI Perl script responding to a GET request.</p>";
print "</body></html>";