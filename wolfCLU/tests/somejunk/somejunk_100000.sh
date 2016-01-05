for i in {1..100000}
do
    if [ $i -eq 1 ]
    then
        echo "This is a file that contains 100,000 lines of text for encryption testing" > somejunk_100000.txt
    else
        echo "This is a file that contains 100,000 lines of text for encryption testing" >> somejunk_100000.txt
    fi
done
