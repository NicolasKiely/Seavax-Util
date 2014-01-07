seatab: src/seatab/seatab.c
	gcc -o seatab src/seatab/seatab.c
	
seatabd: src/seatabd/seatabd.c src/seatabd/net.c src/seatabd/bucket.c src/seatabd/parser.c src/seatabd/bucketString.c src/seatabd/profile.c
	gcc -o seatabd src/seatabd/seatabd.c src/seatabd/net.c src/seatabd/bucket.c src/seatabd/parser.c src/seatabd/bucketString.c src/seatabd/profile.c
