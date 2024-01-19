DEFAULT: weather

weather: intl.wany west.wany wanyload
	./wanyload intl.wany west.wany

wanyload: wanyload.c
	cc -O2 -o $@ $^

intl.wany: intl.data wanywi10.pl wanycode.pl
	perl wanywi10.pl intl.data | perl wanycode.pl > $@

west.wany: west.data wanyww10.pl wanycode.pl
	perl wanyww10.pl west.data | perl wanycode.pl > $@

intl.data:
	curl -s https://tgftp.nws.noaa.gov/data/summaries/selected_cities/current/international.txt > $@

west.data:
	curl -s https://tgftp.nws.noaa.gov/data/summaries/selected_cities/current/western_us.txt > $@

clean:
	rm -f wanyload *.data *.wany
