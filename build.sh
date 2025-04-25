cd ./lib/DPS/lib/c_vector/
make 
cd -
cp ./lib/DPS/lib/c_vector/*.o .

cd ./lib/DPS/
make
cd -
cp ./lib/DPS/*.o .

make
