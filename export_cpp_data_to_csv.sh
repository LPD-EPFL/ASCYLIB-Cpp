#!/bin/bash
for data_file in `ls data/gondolin.*.dat`; do
	echo $data_file
	csv_file=`echo $data_file | sed -r 's/dat$/csv/'`
	cat $data_file | sed -r 's/\s+/,/g' > $csv_file
done

