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

# ($extra & 1 << 22) == 0
  $newanti = 0;
  $newanti2 = 0;
  ($anti & (1 << 1)) && ($newanti |= 1);
  ($anti & (1 << 2)) && ($newanti |= (1 << 1));
  ($anti & (1 << 3)) && ($newanti |= (1 << 3));
  ($anti & (1 << 4)) && ($newanti |= (1 << 4));
  ($anti & (1 << 5)) && ($newanti |= (1 << 5));
  ($anti & (1 << 6)) && ($newanti |= (1 << 6));
  ($anti & (1 << 7)) && ($newanti |= (1 << 7));
  ($anti & (1 << 8)) && ($newanti |= (1 << 8));
  ($anti & (1 << 9)) && ($newanti |= (1 << 9));
  ($anti & (1 << 10)) && ($newanti |= (1 << 10));
  ($anti & (1 << 11)) && ($newanti |= (1 << 11));
  ($anti & (1 << 12)) && ($newanti |= (1 << 2));
  ($anti & (1 << 13)) && ($newanti |= (1 << 16));
  ($anti & (1 << 14)) && ($newanti |= (1 << 13));
  ($anti & (1 << 15)) && ($newanti |= (1 << 14));
  ($anti & (1 << 16)) && ($newanti |= (1 << 15));
  ($anti & (1 << 17)) && ($newanti2 |= (1));
  ($anti & (1 << 18)) && ($newanti2 |= (1 << 3));
  ($anti & (1 << 19)) && ($newanti2 |= (1 << 10));
  ($anti & (1 << 20)) && ($newanti2 |= (1 << 4));
  ($anti & (1 << 21)) && ($newanti2 |= (1 << 6));
  ($anti & (1 << 22)) && ($newanti2 |= (1 << 7));
  ($anti & (1 << 23)) && ($newanti2 |= (1 << 1));
  ($anti & (1 << 24)) && ($newanti2 |= (1 << 5));
  ($anti & (1 << 25)) && ($newanti2 |= (1 << 2));
  ($anti & (1 << 26)) && ($newanti2 |= (1 << 9));
  ($anti & (1 << 27)) && ($newanti2 |= (1 << 8));
  ($anti & (1 << 28)) && ($newanti2 |= (1 << 11));
  ($anti & (1 << 29)) && ($newanti2 |= (1 << 12));

  ($anti2 & 1) && ($newanti2 |= (1 << 13));
  ($anti2 & (1 << 1)) && ($newanti2 |= (1 << 14));
  ($anti2 & (1 << 2)) && ($newanti2 |= (1 << 15));
  ($anti2 & (1 << 3)) && ($newanti2 |= (1 << 16));
  ($anti2 & (1 << 9)) && ($newanti2 |= (1 << 19));
  ($anti2 & (1 << 10)) && ($newanti2 |= (1 << 20));
  ($anti2 & (1 << 11)) && ($newanti2 |= (1 << 21));
  ($anti2 & (1 << 12)) && ($newanti2 |= (1 << 23));
  ($anti2 & (1 << 13)) && ($newanti |= (1 << 17));
  ($anti2 & (1 << 14)) && ($newanti |= (1 << 18));
  ($anti2 & (1 << 15)) && ($newanti2 |= (1 << 22));
  ($anti2 & (1 << 16)) && ($newanti2 |= (1 << 24));
  ($anti2 & (1 << 17)) && ($newanti2 |= (1 << 25));
  ($anti2 & (1 << 18)) && ($newanti2 |= (1 << 26));
  ($anti2 & (1 << 19)) && ($newanti |= (1 << 22));
  ($anti2 & (1 << 20)) && ($newanti |= (1 << 20));
  ($anti2 & (1 << 21)) && ($newanti |= (1 << 21));
  ($anti2 & (1 << 22)) && ($newanti |= (1 << 19));
  ($anti2 & (1 << 23)) && ($newanti |= (1 << 23));
  ($anti2 & (1 << 24)) && ($newanti |= (1 << 24));
  ($anti2 & (1 << 25)) && ($newanti |= (1 << 25));

  if ($anti & 1) {
    $newanti = 0;
    $newanti2 = 0;
  } else {
    $extra |= (1 << 10);
  }

  $flag_count = 0;
  for ($x = 0; $x < 32; $x++) {
    if ($newanti2 & (1 << $x) ) {
      $flag_count++;
    }
  }

  if ($flag_count > 16) {
    $newanti2 = ~0 - $newanti2;
    $newanti2 &= ~0 - (1 << 31 | 1 << 30 | 1 << 29); 
    $extra |= (1 << 9);
  }

  $flag_count = 0;
  for ($x = 0; $x < 32; $x++) {
    if ($newanti & (1 << $x) ) {
      $flag_count++;
    }
  }

  if ($flag_count > 13) {
    $newanti = ~0 - $newanti;
    $extra &= ~(1 << 10);
  }
  $anti = $newanti;
  $anti2 = $newanti2;

  print "$type $material $size $space $craft $damres $extra $wear $extra2 $anti $anti2\n";
  @values = split(' ', <>);
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

