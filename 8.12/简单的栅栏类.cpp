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
	��ÿһ���У�ÿһ���̶߳���դ��������wait()������֤�߳���������һ�£�
	���ҵ������̶߳���������ô���һ���̻߳����done_waiting()������count��ֵ��
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
��һ�������ڵ�Ԫ��(����Ϊ1)��Ӻ�(��֮ǰһ��)��֮��;���Ϊ2��Ԫ����ӣ��ں����;���Ϊ
4��Ԫ����ӣ��Դ����ơ����磬��ʼ����Ϊ[1��2��3��4��5��6��7��8��9]����һ�κ�Ϊ
[1��3��5��7��9��11��13��15��17]���ڶ��κ�Ϊ[1��3��6��10��14��18,	22��26��30]��
��һ�ξ�Ҫ��4��Ԫ���ˡ������κ�[1,	3,	6,	10,	15,	21,	28,36,	44]����һ�ξ�Ҫ��
8��Ԫ���ˡ����Ĵκ�[1,	3,	6,	10,	15,	21,	28,	36,	45]����������յĽ����
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