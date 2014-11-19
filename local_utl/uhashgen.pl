#!/usr/bin/perl
#perl uhashgen.pl < userlist.txt > uhashgen.dat
#userlist.txt是用户名列表，一行一个
#输出为uhashgen.dat, 并产生头文件uhashgen.h
#uhashgen.h大概应该拷贝到libythtbbs/
#uhashgen.dat应该拷贝到~bbs下，之后需要重启动，以便shminit可以重建共享内存
sub mytoupper
{
	($c) = @_;
	$c = ord($c);
	if ($c <= ord("Z") && $c >= ord("A")) {
		return chr($c);
	}
	if ($c <= ord("z") && $c >= ord("a")) {
		return chr($c+ord("A")-ord("a"));
	}
	return chr($c % (ord("Z")-ord("A")) + ord("A"))
}
sub docount
{
	(my $prefix1) = @_;
	my @h;
	$line++;
	my $myl = $line;
	for (my $i=ord("A"); $i<=ord("Z"); $i++) {
		my $c = chr($i);
		my $prefix = $prefix1.$c;
		my $count = $cc{$prefix};
		if (length($prefix) <= 4 && $count / $hsize > ($alluser / $max_hash)) {
			$l = docount($prefix);
			$l[$myl][$i-ord("A")] = -$l-1;
		} else {
			$l[$myl][$i-ord("A")] = $hn;
			$hn++;
		}
	}
	return $myl;
}
sub prime
{
	(my $num) = @_;
	for (my $i=2; $i<=sqrt($num);$i++) {
		if ($num % $i == 0) {
			return 0;
		}
	}
	return 1;
}
#main
@userid=<>;
foreach $userid (@userid) {
	chomp $userid;
	$l = length($userid);
	for ($i = 0; $i < $l; $i++) {
		substr($userid, $i, 1) = mytoupper(substr($userid, $i, 1));
	}
	for ($i = 1; $i <= 4; $i++) {
		if (length($userid) >= $i) {
			$cc{substr($userid, 0, $i)}++;
		}
	}
}
$alluser = scalar(@userid);
$min_s = 100000;
for ($kkk = 2000000; $kkk <= 2000000; $kkk++){
	for ($hhh = 100; $hhh <=1000; $hhh+=1) {
		if (!prime($hhh)) {
			next;
		}
		$max_hash = $kkk;
		$hsize = $hhh;
		$hn = 0;
		$line = -1;
		@l = ();
		$tt = 0;
		docount("");
		%ccc = ();
		foreach $userid (@userid) {
			$i = 0;
			$len = length($userid);
			$n1 = $l[0][ord(substr($userid, $i, 1))-ord("A")];
			while ($n1<0) {
				$n1 = -$n1 - 1;
				if ($i == $len - 1) {
					$n1 = $l[$n1][0];
				} else {
					$i++;
					$n2 = ord(substr($userid, $i, 1));
					$n1 = $l[$n1][$n2 - ord("A")];
				}
			}
			$n1 = ($n1 * $hsize) % $max_hash + 1;
			if ($i == $len - 1) {
				$hash = $n1;
			} else {
				$n2 = 0;
				$len1 = $len;
				while ($i < $len) {
					$n2 += $len1 * (ord(substr($userid, $i, 1)) - ord("A"));
					$i++;
					$len1 --;
				}
				$hash = ($n1 + $n2 % $hsize) % $max_hash + 1;
			}
			$ccc{$hash}++;
		}
		$s = 0;
		$i = 0;
		foreach (keys %ccc) {
			if ($ccc{$_} > 0) {
				$tt++;
			}
			$s+=$ccc{$_}*(1+$ccc{$_})/2;
		}
		
		$ee = ($alluser)/$tt;
		$e = $s/$alluser;
		
		if ($min_s > $e) {
			$min_s = $e;
			$min_e = $e;
			$min_k = $kkk;
			$min_h = $hhh;
		}
	}
}
$max_hash = $kkk;
$hsize = $min_h; 
$hn = 0;
$line = -1;
@l = ();
docount("");
for ($k=0; $k<=$line; $k++) {
	for ($i=0;$i<=26;$i++) {
		print $l[$k][$i]." ";
	}
	print "\n";
}
open(fd, ">uhashgen.h");
print fd "#define UCACHE_HASHSIZE  (MAXUSERS*4)\n";
print fd "#define UCACHE_HASHBSIZE ($hsize)\n";
print fd "#define UCACHE_HASHLINE ($line+1)\n";
print fd "// ($min_s)\n";
