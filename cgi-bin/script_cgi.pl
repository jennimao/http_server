#!/usr/bin/perl

use strict;
use warnings;

# Print the CGI header with a "200 OK" response and content type
print "Content-Type: text/plain\n\n";

# Read the POST data from the standard input (STDIN)
my $post_data;
while (<STDIN>) {
    $post_data .= $_;
}

# Print the received POST data in the response
print "Received POST data:\n";
print $post_data;