all:
	rm -rf a.out
	c++ *.cpp -fsanitize=address