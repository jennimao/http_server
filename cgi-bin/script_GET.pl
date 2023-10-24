#!/usr/bin/perl

use strict;
use warnings;

use CGI;

# Create a CGI object to parse query parameters
my $cgi = CGI->new;

# Get query parameters from the URL
my $name = $cgi->param('name');
my $age = $cgi->param('age');

# Set the content type to HTML
print $cgi->header('text/html');

# Generate an HTML response
print "<html>";
print "<head><title>GET Request CGI</title></head>";
print "<body>";
print "<h1>GET Request CGI Response</h1>";
print "<p>Name: $name</p>";
print "<p>Age: $age</p>";
print "</body>";
print "</html>";