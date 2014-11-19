#!/usr/bin/perl -w
use strict;
my(
        $sum
);
$sum=0;
while(<STDIN>){
        $sum+=$_;
}
printf"\t\t%3d\t\t",$sum/240;

