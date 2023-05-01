#!/bin/sh
# --------------------------------------------------
# If there is an old set of instructions, delete it.  Next, read the run
# details and generate an updated set of instructions.
rm -f ./temp.sh
cd ./pyth
python3 prepare_details.py "../run_details.csv" > ../temp.sh
cd ../
# Run the instructions just generated, then remove the temporary file.
sh ./temp.sh
rm -f temp.sh
echo "Done.";
