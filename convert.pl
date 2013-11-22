#Utility to convert from xfig to C
#
# 1. In xfig, export as metafont (hey, it works, don't knock it)
# 2. Run perl <icons.mf >icons.h
#

$max = 0.0;
$min = 1000000000000.0;
$list = [];

$minY = 100000000000000.0;
$maxY = 0.0;

while($_ = <STDIN>) {
  if (/\(([0-9.]*), ([0-9.]*)\)/) {
    @{$list} = (@{$list}, [$1,$2]);
    if ($1 < $min) { $min = $1 }
    if ($1 > $max) { $max = $1 }
    if ($2 < $minY) { $minY = $2 }
    if ($2 > $maxY) { $maxY = $2 }
  }
  elsif (@{$list}) {
    @all = (@all, [$min, $max, $list]);
    $list = [];
    $max = 0.0;
    $min = 1000000000000.0;
  }
}

$scale = 1.0 / ($maxY - $minY);

@all = sort { $a->[0] <=> $b->[0] } @all;

@all = (@all, [1e10,1e10,[]]);

$offset = 1000000.0;
$lastEnd = -100.0;
@lines = ();
@widths = ();
$numIcon = 0;
foreach $i (@all) {
  if ($i->[0] > $lastEnd and @lines) {
    print "double Icon".$numIcon."[][4] = {\n";
    $numIcon++;

    #$offset = $i->[0];

    foreach $line (@lines) {
      $count = 0;
      $xx = ($line->[0]+$line->[2]) / 2.0;
      $yy = ($line->[1]+$line->[3]) / 2.0;
      foreach $line2 (@lines) {
        if ($line != $line2 and
	    $line2->[1] <= $yy and
	    $line2->[3] > $yy) {
	  $x =
	    $line2->[0] +
	    ($line2->[2]-$line2->[0])/($line2->[3]-$line2->[1])
	    * ($yy - $line2->[1]);
	  if ($x < $xx) {
	    $count ++;
	  }
	}
      }
      #print $count;
      if ($count%2 != 0) {
        print "  {".
	  ($line->[0])*$scale .",".
	  (1.0-($line->[1]-$minY)*$scale) .",".
	  ($line->[2])*$scale .",".
	  (1.0-($line->[3]-$minY)*$scale) ."},\n";
      } else {
        print "   {".
	  ($line->[2])*$scale .",".
	  (1.0-($line->[3]-$minY)*$scale) .",".
	  ($line->[0])*$scale .",".
	  (1.0-($line->[1]-$minY)*$scale) ."},\n";
      }
    }

    print "  {0,0,0,0}\n};\n\n";

    @lines = ();

    @widths = (@widths, ($lastEnd-$offset)*$scale);
    $offset = 1000000000.0;
  }

  if ($lastEnd < $i->[1]) {
    $lastEnd = $i->[1];
  }
  if ($offset > $i->[0]) {
    $offset = $i->[0];
  }

  for($j=1;$j<(@{$i->[2]});$j++) {
    $list = [$i->[2]->[$j-1]->[0] - $offset , $i->[2]->[$j-1]->[1] ,
             $i->[2]->[$j]->[0] - $offset , $i->[2]->[$j]->[1]];
    if ($list->[1] > $list->[3]) {
      $list = [$list->[2],$list->[3],$list->[0],$list->[1]];
    }
    #print join(",",@{$list}) . "\n";
    if ($list->[1] != $list->[3]) {
      @lines = (@lines, $list);
    }
  }
}

print "typedef double LineType[4];\n"; 
print "typedef LineType *IconType;\n"; 
print "IconType Icons[] = {\n";
for($i = 0;$i < $numIcon;$i++) {
  print "  Icon$i,\n";
}
print "};\n\n";

print "double IconWidths[] = {\n";
foreach $i (@widths) { print "  $i,\n"; }
print "};\n";
