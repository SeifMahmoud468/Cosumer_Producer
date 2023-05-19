$(CC) = gcc
final:
	@g++ producers.cpp -o producer
	@g++ consumer.cpp -o consumer
clean:
	@rm consumer
	@rm producer
	@ipcrm -a