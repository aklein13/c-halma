ipcs -s | grep akane | perl -e 'while (<STDIN>) { @a=split(/\s+/); print `ipcrm -q $a[1]`}; print `ipcrm -m $a[1]`; print `ipcrm -s $a[1]`'
ipcs -m | grep akane | perl -e 'while (<STDIN>) { @a=split(/\s+/); print `ipcrm -q $a[1]`}; print `ipcrm -m $a[1]`; print `ipcrm -m $a[1]`'
