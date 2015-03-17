for i in {1..10000}
do
    if [ $i -eq 1 ]
    then
        echo "This is a file that contains 10,000 lines of text for encryption testing" > somejunk_10000.txt
    else
        echo "This is a file that contains 10,000 lines of text for encryption testing" >> somejunk_10000.txt
    fi
done
