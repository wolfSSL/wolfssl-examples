for i in {1..10}
do
    if [ $i -eq 1 ]
    then
        echo "This is a file that contains 10 lines of text for encryption testing" > somejunk_10.txt
    else
        echo "This is a file that contains 10 lines of text for encryption testing" >> somejunk_10.txt
    fi
done
