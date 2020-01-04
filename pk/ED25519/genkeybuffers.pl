#!/usr/bin/perl

# gencertbuf.pl
# version 1.1
# Updated 04/11/2017
#
# Copyright (C) 2006-2020 wolfSSL Inc.
#

use strict;
use warnings;

# ---- SCRIPT SETTINGS -------------------------------------------------------

# output C header file to write cert/key buffers to
my $outputFile = "./test_keys.h";

# 2048-bit certs/keys to be converted
# Used with USE_CERT_BUFFERS_2048 define.

my @fileList_2048 = (
        [ "./ed_pub_test_key.der", "ed_pub_key_der_32" ],
        [ "./ed_priv_test_key.der", "ed_priv_key_der_64" ],
        );

# ----------------------------------------------------------------------------

my $num_2048 = @fileList_2048;

# open our output file, "+>" creates and/or truncates
open OUT_FILE, "+>", $outputFile  or die $!;

print OUT_FILE "/* certs_test.h */\n\n";
print OUT_FILE "#ifndef WOLFSSL_CERTS_TEST_H\n";
print OUT_FILE "#define WOLFSSL_CERTS_TEST_H\n\n";


# convert and print 32/64-bit certs/keys
print OUT_FILE "#ifdef USE_CERT_BUFFERS_ED\n\n";
for (my $i = 0; $i < $num_2048; $i++) {

    my $fname = $fileList_2048[$i][0];
    my $sname = $fileList_2048[$i][1];

    print OUT_FILE "/* $fname, (32/64)-bit */\n";
    print OUT_FILE "static const unsigned char $sname\[] =\n";
    print OUT_FILE "{\n";
    file_to_hex($fname);
    print OUT_FILE "};\n";
    print OUT_FILE "static const int sizeof_$sname = sizeof($sname);\n\n";
}


print OUT_FILE "#endif /* USE_CERT_BUFFERS_ED */\n\n";
print OUT_FILE "#endif /* WOLFSSL_CERTS_TEST_H */\n\n";
# close certs_test.h file
close OUT_FILE or die $!;

# print file as hex, comma-separated, as needed by C buffer
sub file_to_hex {
    my $fileName = $_[0];

    open my $fp, "<", $fileName or die $!;
    binmode($fp);

    my $fileLen = -s $fileName;
    my $byte;

    for (my $i = 0, my $j = 1; $i < $fileLen; $i++, $j++)
    {
        if ($j == 1) {
            print OUT_FILE "\t";
        }
        read($fp, $byte, 1) or die "Error reading $fileName";
        my $output = sprintf("0x%02X", ord($byte));
        print OUT_FILE $output;

        if ($i != ($fileLen - 1)) {
            print OUT_FILE ", ";
        }

        if ($j == 10) {
            $j = 0;
            print OUT_FILE "\n";
        }
    }

    print OUT_FILE "\n";

    close($fp);
}

