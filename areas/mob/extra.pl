#!/usr/bin/perl -w

while ($line = <>) {
  if ($line =~ /^([0-9]+) (.*)/) {
    $extra = $1;
    $rest = $2;
    $extra = $extra & (~0 ^ 1<<24);
    print "$extra $rest\n";
  } else {
    print "$line";
  }
}
