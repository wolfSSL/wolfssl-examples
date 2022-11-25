#!/bin/bash

mkdir pqm4_kyber512
mkdir Inc
mkdir tmp

cur_dir=$PWD
cd pqm4_kyber512
code_dir=$PWD
cd ../Inc
inc_dir=$PWD
cd ../tmp
tmp_dir=$PWD

git clone https://github.com/mupq/pqm4.git $tmp_dir/pqm4
cd $tmp_dir/pqm4
git checkout 1eeb74e4106a80e26a9452e4793acd6f191fe413

git clone https://github.com/mupq/mupq.git $tmp_dir/mupq
cd $tmp_dir/mupq
git checkout 3b48fa5aff6f5921df5b3444450281daca6d21d1

git clone  https://github.com/pqclean/pqclean.git $tmp_dir/pqclean
cd $tmp_dir/pqclean
git checkout 2cc6e744e63c9aa7703ba3ae3304215e83bba167

# Populate include directory
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/params.h $inc_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/api.h $inc_dir

# Get what we need from pqclean
cp $tmp_dir/pqclean/common/randombytes.c $code_dir
cp $tmp_dir/pqclean/common/randombytes.h $code_dir

# Get what we need from mupq
cp $tmp_dir/mupq/common/keccakf1600.c $code_dir
cp $tmp_dir/mupq/common/keccakf1600.h $code_dir
cp $tmp_dir/mupq/common/fips202.c $code_dir
cp $tmp_dir/mupq/common/fips202.h $code_dir

# Get what we need from pqm4
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/cbd.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/cbd.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/fastaddsub.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/fastbasemul.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/fastinvntt.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/fastntt.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/indcpa.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/indcpa.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/kem.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/macros.i $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/matacc.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/matacc.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/matacc.i $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/matacc_asm.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/ntt.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/ntt.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/poly.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/poly.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/poly_asm.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/polyvec.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/polyvec.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/reduce.S $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/symmetric-fips202.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/symmetric.h $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/verify.c $code_dir
cp $tmp_dir/pqm4/crypto_kem/kyber512/m4fspeed/verify.h $code_dir

# Rename some files
mv $inc_dir/api.h $inc_dir/api_kyber.h

# Patch in some small fixups.
cd $cur_dir
patch -p0 < setup_kyber.patch

# give the user some instructions
echo "Created $code_dir . Please add it to your project's Core directory."
echo "Created $inc_dir . Please add the files it contains to you project's Core/Inc directory."

# Cleanup
rm -rf $tmp_dir
