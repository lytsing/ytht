#!/usr/bin/perl
sub log_f
{	
	if (ref($_[0]) eq "HASH" ){
		print log_file "HASH:{\n";
		foreach $i (keys %{$_[0]}){
			print log_file $i.": ";
			&log_f($_[0]->{$i});
		}
		print log_file "}\n";
	} elsif (ref($_[0]) eq "ARRAY" ){
		print log_file "ARRAY:[\n";
		foreach $i (@{$_[0]}){
			&log_f($i);
		}
		print log_file "]\n";
	} else {
		print log_file $_[0]."\n";
	}
}

sub check_ip
{
	my $i,$j;
	for $i (keys %iplist) {
		for $j (@{$iplist{$i}}) {
			return $i if( ($_[0] >= $j->[0]) and ($_[0] <= $j->[1]));
		}
	}
	return "other";
}

*log_file=\*STDOUT;

open IP, "< ip.txt" or die "faint,can\'t open data file!";
@lines=<IP>;
close IP;
foreach $line (@lines) {
	if($line =~ /(\d+)\.(\d+)\.(\d+)\.(\d+)-(\d+)\.(\d+)\.(\d+)\.(\d+)/){
		if (not defined($reg)){
			print "faint " && exit -1;
		}
		if (not defined($iplist{$reg})){
			$iplist{$reg}=[];
		}
		push (@{$iplist{$reg}},[$1 * 65536* 256 +$2*65536+$3 * 256 +$4 ,$5 * 65536* 256 +$6*65536+$7 * 256 +$8]);
	} elsif ( $line =~ /region=(\S+)/ ) {
		$reg=$1;
	}
}

foreach $i (keys %count) {
	print $i.":".$count{$i};
}
open IP, "< $ARGV[0]" or die "faint, can\'t open input file!";
@lines=<IP>;
close IP;
$total=0;
foreach $line (@lines) {
	if ($line =~ /(\d+)\.(\d+)\.(\d+)\.(\d+)/){
		print $1.".".$2.".".$3.".".$4."\n";
		$dist=&check_ip($1 * 65536* 256 +$2*65536+$3 * 256 +$4);
		$count{$dist}++;
		$total++;
	}
}
foreach $i (keys %count) {
	print $i.": ".$count{$i}." ".($count{$i}*100/$total)."%\n";
}
	print $total;
