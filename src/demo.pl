#!/usr/bin/perl

use warnings;
use strict;

use List::Util 'shuffle';

open WORDS, "/usr/share/dict/words" or die $!;
open PATCH, ">patch" or die $!;

my $count = 0;
my @words;
while (<WORDS>) {
    chomp $_;
    print PATCH "+" . length($_) . "," . length($_) . ":" . $_ . "->" . $_ . "\n";
    $count += 1;

    push(@words, $_);
}
close(PATCH);


open QUERY, ">query" or die $!;
for (my $queries = 0; $queries < 1000000; $queries++)
{
    my $word = $words[ rand($#words) ] ;
    print QUERY length($word) . ":$word\n"; 
}
close(QUERY);

unlink("dict.db");

# Create the dict db
print "Creating DB file with $count records\n";
system("time ./nutrient_patch patch dict.db");

# Query the DN
print "Querying the DB file for 10,000,000 random records\n";
system("time ./nutrient_query query dict.db > /dev/null");

