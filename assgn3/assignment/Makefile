OBJS = main.o core.o process.o system.o scheduler.o

sim: $(OBJS)
	g++ $(OBJS) -o sim

$(OBJS): %.o: %.cpp
	g++ -c $^ -o $@

clean:
	rm *.o sim
