ccpfsk-fancontrol: ccpfsk-fancontrol.c
	$(CC) -o $@ -c $<

clean:
	rm -f ccpfsk-fancontrol
