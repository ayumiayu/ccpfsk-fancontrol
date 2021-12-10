ccpfsk-fancontrol: ccpfsk-fancontrol.c
	$(CC) $< -o $@

clean:
	rm -f ccpfsk-fancontrol
