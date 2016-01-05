for i in {1..1000}
do
    if [ $i -eq 1 ]
    then
        echo "This is a file that contains 1,000 lines of text for encryption testing" > somejunk_1000.txt
    else
        echo "This is a file that contains 1,000 lines of text for encryption testing" >> somejunk_1000.txt
    fi
done
