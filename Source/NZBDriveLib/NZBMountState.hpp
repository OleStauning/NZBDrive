#ifndef NZBMOUNTSTATE_HPP
#define NZBMOUNTSTATE_HPP



namespace ByteFountain
{
	struct NZBMountState
	{
		typedef boost::signals2::signal<void()> CancelSignal;
		enum State { Mounting, Mounted, Canceling };
		
		int32_t nzbID;
		CancelSignal cancel;
		State state;
	
		NZBMountState(int32_t nzbID): 
			nzbID(nzbID),cancel(),state(Mounting)
		{}
	};
}

#endif
