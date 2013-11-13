#!/usr/bin/perl -w
$line = <>;
while ($line && ($line =~ /#([0-9]*)\n/)) {
  $number = $1;
  print "#$number\n";
  $keywords = <>;
  print $keywords;
  $short = <>;
  print $short;
  $long = <>;
  print $long;
  $line = <>;
  print $line;
  @first = split(' ', <>);
  $type = $first[0];
  $material = $first[1];
  $size = $first[2];
  $space = $first[3];
  $craft = $first[4];
  $damres = $first[5];
  $extra = $first[6];
  $wear = $first[7];
  $extra2 = $first[8];
  $anti = $first[9];
  $anti2 = $first[10];

  print "$type $material $size $space $craft $damres $extra $wear $extra2 $anti $anti2\n";
  @values = split(' ', <>);

  if ($type == 6) {
    $speed = $values[1] * 35;
    $range = $values[0] * 30;
    if ($speed < 60) {
      $speed = 60;
    } elsif ($speed > 150) {
      $speed = 150;
    }
    if ($range < 60) {
      $range = 60;
    } elsif ($range > 150) {
      $range = 150;
    }
    $values[0] = $speed;
    $values[1] = $range;
  }

  print join(' ', @values),"\n";
  @third = split(' ', <>);
  $weight = $third[0];
  $cost = $third[1];
  $condition = $third[2];
  $bitvector1 = 0;
  $bitvector2 = 0;
  $bitvector3 = 0;
  $bitvector4 = 0;
  if ($#third >= 3) {
    $bitvector1 = $third[3];
    if ($#third >= 4) {
      $bitvector2 = $third[4];
      if ($#third >= 5) {
        $bitvector3 = $third[5];
        if ($#third >= 6) {
          $bitvector4 = $third[6];
        }
      }
    }
  }
  if ($#third >= 3) {
     print "$weight $cost $condition $bitvector1 $bitvector2 $bitvector3 $bitvector4\n";
  } else {
     print "$weight $cost $condition\n";
  }
  $line = <>;
  while ($line && ($line eq "E\n")) {
    print "E\n";
    while (($line = <>) ne "~\n") {
      print $line;
    }
    print $line;
    $line = <>;
  }
  while ($line && ($line eq "A\n")) {
    print "A\n";
    $line = <>;
    print $line;
    $line = <>;
  }
  while ($line && ($line eq "T\n")) {
    print "T\n";
    $line = <>;
    print $line;
    $line = <>;
  }
}

