echo Name to look up:
read NAME
if grep $NAME $HOME/phonelist
then
echo there is the name
else
echo cannot locate $NAME in our records
fi
