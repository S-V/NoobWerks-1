#pragma once

/// A small class to encapsulate calculating a cumulative moving average
template<typename T>
class TSimpleMovingAverage
{
public_internal:
	T CA;
	T Count;

public:
	TSimpleMovingAverage()
		: CA(T(0))
		, Count(T(0))
	{}

	// Given the latest sample value, this returns the moving average for all 
	// values up to the latest specified.
	T calculate(const T& value)
	{
		Count += T(1);
		CA = CA + ((value - CA) / Count);
		return CA;
	}
};

typedef TSimpleMovingAverage<float> SimpleMovingAverageF;



/// Calculates Simple Moving Average (SMA) - the unweighted mean of the previous n data samples.
/// Taken from http://stackoverflow.com/a/87732/4232223
template< int N >
class TMovingAverageN
{
	int		m_tickIndex;
	float	m_tickSum;
	float	m_samples[N];	//!< samples for calculating simple moving average (SMA)

public:
	TMovingAverageN()
	{
		this->Reset();
	}
	void Reset()
	{
		/* need to zero out the buffer before starting */
		/* average will ramp up until the buffer is full */
		mxZERO_OUT(*this);
	}
	/// returns average ticks per frame over the N last frames
	float add( float _sample )
	{
		m_tickSum -= m_samples[ m_tickIndex ];	// subtract value falling off
		m_tickSum += _sample;	// add new value
		m_samples[ m_tickIndex ] = _sample;// save new value so it can be subtracted later
		m_tickIndex = ++m_tickIndex % N;	// inc buffer index
		return this->average();
	}
	float average() const
	{
		return m_tickSum / N;
	}
};

// last_n - circular buffer class
// Part of lvvlib - https://github.com/lvv/lvvlib
// Copyright (c) 2000-2013
// Leonid Volnitsky (Leonid@Volnitsky.com)
template<class T, size_t N>
struct last_n {
	size_t oldest;
	T ring[N];
	last_n() { oldest = 0; }
	void push_back(T x)  { ring[oldest++] = x;   oldest %= N; };
	T prev(size_t i)     { return ring[(oldest-1-i+N)%N];};
};

mxSTOLEN("https://github.com/nasa/libSPRITE/blob/master/util/Sample_set.h");
/**
 * This class keeps track of the last 'n' samples of data.
 */
template <typename T> class Sample_set {

  public:
    /**
     * Constructor.
     * @param nelem Number of elements that can be stored in the
     * buffer.
     * @satisfies{util-2.1}
     */
    Sample_set(unsigned int nelem)
        : m_elem(static_cast<T *>(malloc(sizeof(T) * nelem)))
        , m_nelem((NULL == m_elem) ? 0 : nelem)
        , m_head(nelem - 1)
        , m_npop(0)
    {
    }

    /**
     * Destructor.
     */
    ~Sample_set()
    {
        m_nelem = m_npop = 0;
        free(m_elem);
        m_elem = NULL;
    }

    /**
     * Return indication as to whether this class is valid.
     * @return True if valid, else false.
     */
    bool is_valid() const
    {
        static bool valid = m_nelem != 0;
        return valid;
    }

    /**
     * Return the number of elements in a sample set.
     * @return Number of elements or 0 if invalid.
     * @satisfies{util-2.2}
     */
    unsigned int nelem() const
    {
        return m_nelem;
    }

    /**
     * Test if the sample set is full.
     * @return True if full, else false.
     * @satisfies{util-2.3}
     */
    bool is_full() const
    {
        return m_nelem == m_npop;
    }

    /**
     * Return the number of elements populated.
     * @return The number of elements that have been populated.
     * @satisfies{util-2.4}
     */
    unsigned int npopulated() const
    {
        return m_npop;
    }

    /**
     * Operator overload for accessing an element.
     * Used like this: double x = S(i);
     * @note The most recent element is 1.
     * @param i Element to access.
     * @return Eelement value.
     */
    T operator()(unsigned int i) const
    {
        assert((i >= 1) && (i <= m_nelem));

        int elem = (m_head - i + 1);
        if(elem < 0) {
            elem += m_nelem;
        }
        return m_elem[elem];
    }

    /**
     * add a new sample to the sample set. If the sample set is full,
     * the oldest sample will be replaced.
     * @param val Sample value to store.
     * @satisfies{util-2.5}
     * @satisfies{util-2.6}
     */
    void push(const T val)
    {
        /* Move the head pointer.
         */
        m_head = (m_head + 1) % m_nelem;

        /* add the new sample.
         */
        m_elem[m_head] = val;

        /* Increment the number of samples populated if necessary.
         */
        if(m_npop < m_nelem) {
            ++m_npop;
        }
    }

  private:
    /**
     * Copy constructor.
     * The copy constructor is made private to prevent copy because the
     * class has a pointer member variable.
     */
    Sample_set(const Sample_set &);

    /**
     * Assignment operator.
     * The assignment operator is made private to because the class has a
     * pointer member variable.
     */
    Sample_set &operator=(const Sample_set &);

    /**
     * Buffer elements.
     */
    T *m_elem;

    /**
     * Number of elements.
     */
    unsigned int m_nelem;

    /**
     * Index of the first element.
     */
    unsigned int m_head;

    /**
     * Number of sample positions populated.
     */
    unsigned int m_npop;
};
