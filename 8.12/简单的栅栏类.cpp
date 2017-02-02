#include <atomic>
#include <thread>
#include <vector>

class barrier
{
	std::atomic<unsigned> count;
	std::atomic<unsigned> spaces;
	std::atomic<unsigned> generation;

	barrier(unsigned count_):
		count(count_),spaces(count_),generation(0)
	{}

	void wait()
	{
		unsigned const gen = generation.load();
		if (!--spaces)
		{
			spaces = count.load();
			++generation;
		}
		else
		{
			while (generation == gen)
				std::this_thread::yield();
		}
	}
	/*
	在每一步中，每一个线程都在栅栏出调用wait()，来保证线程所处步骤一致，
	并且当所有线程都结束，那么最后一个线程会调用done_waiting()来减少count的值。
	*/
	void done_waiting()
	{
		--count;
		if (!--spaces)
		{
			spaces = count.load();
			++generation;
		}
	}
};

/*
第一次与相邻的元素(距离为1)相加和(和之前一样)，之后和距离为2的元素相加，在后来和距离为
4的元素相加，以此类推。比如，初始序列为[1，2，3，4，5，6，7，8，9]，第一次后为
[1，3，5，7，9，11，13，15，17]，第二次后为[1，3，6，10，14，18,	22，26，30]，
下一次就要隔4个元素了。第三次后[1,	3,	6,	10,	15,	21,	28,36,	44]，下一次就要隔
8个元素了。第四次后[1,	3,	6,	10,	15,	21,	28,	36,	45]，这就是最终的结果。
*/
template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{
	typedef typename Iterator::value_type value_type;
	struct process_element //1
	{
		void operator()(Iterator first, Iterator last,
			std::vector<value_type>& buffer,
			unsigned i, barrier& b)
		{
			value_type& ith_element = *(first + i);
			bool updata_source = false;

			for (unsigned step = 0; stride = 1; stride <= i; ++step, stride *= 2)
			{
				value_type const& source = 
					(step % 2) ? buffer[i] : ith_element;//2

				value_type const& dest = 
					(step % 2) ? ith_element : buffer[i];

				value_type const& addend = //3
					(step % 2) ? buffer[i - stride] : *(first + i - stride);

				dest = source + addend;//4
				updata_source = !(step % 2);
				b.wait();//5
			}
			if (updata_source)//6
				ith_element = buffer[i];
			b.done_waiting();//7
		}
	};

	unsigned long const length = std::distance(first, last);

	if (length <= 1)
		return;

	std::vector<value_type> buffer(length);
	barrier b(length);

	std::vector<std::thread> threads(length - 1);//8
	join_threads joiner(threads);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (length - 1); ++i)
	{
		threads[i] = std::thread(process_element(), first, last,
			std::ref(buffer), i, std::ref(b));//9
	}
	process_element()(first, last, buffer, length - 1, b);//10

}
int main()
{

}