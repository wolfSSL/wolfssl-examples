for i in {1..100}
do
    if [ $i -eq 1 ]
    then
        echo "This is a file that contains 100 lines of text for encryption testing" > somejunk_100.txt
    else
        echo "This is a file that contains 100 lines of text for encryption testing" >> somejunk_100.txt
    fi
done
