/*
 File: ContFramePool.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 Constructor: Initialize all frames to FREE, except for any frames that you
 need for the management of the frame pool, if any.

 get_frames(_n_frames): Traverse the "bitmap" of states and look for a
 sequence of at least _n_frames entries that are FREE. If you find one,
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.

 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.

 needed_info_frames(_n_frames): This depends on how many bits you need
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.

 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/
ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char mask = 0x3 << ((_frame_no % 4) * 2);

    switch ((bitmap[bitmap_index] & mask))
    {
        case 0:
            return FrameState::Used;
            break;
        case 1:
            return FrameState::Free;
            break;
        case 2:
            return FrameState::HoS;
            break;
        case 3:
            return FrameState::InA;
            break;
    }
}

void ContFramePool::set_state(unsigned long _frame_no, ContFramePool::FrameState _state)
{
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char free_mask = 0x3 << ((_frame_no % 4) * 2);
    unsigned char hos_mask = 0x1 << ((_frame_no % 4) * 2);
    unsigned char ina_mask = 0x2 << ((_frame_no % 4) * 2);


    switch (_state)
    {
        case FrameState::Free:
            bitmap[bitmap_index] |= free_mask;
            break;
        case FrameState::Used:
            bitmap[bitmap_index] ^= free_mask;
            break;
        case FrameState::HoS:
            bitmap[bitmap_index] ^= hos_mask;
            break;
        case FrameState::InA:
            bitmap[bitmap_index] ^= ina_mask;
            break;
    }
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    /*The number of frames must be less than FRAME_SIZE * 8 / 2 to fit the
     management info into 1 frame*/
    assert(_n_frames <= (FRAME_SIZE * 8) / 2); // Divided by 2 because we use 2 bits per frame to represent the current state of the frame.

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;

    /*Initalize the management info frame details*/
    /*If info_frame_no is 0 then make the first frame as info frame, else mark the given frame as info_frame*/
    if (info_frame_no == 0)
    {
        bitmap = (unsigned char *)(base_frame_no * FRAME_SIZE);
    }
    else
    {
        bitmap = (unsigned char *)(info_frame_no * FRAME_SIZE);
    }

    /*Mark the frames as free*/
    for (int fno = 0; fno < _n_frames; fno++)
    {
        set_state(0, FrameState::Free);
    }

    /*Mark the first frame as being used*/
    if (_info_frame_no == 0)
    {
        set_state(0, FrameState::Used);
        nFreeFrames--;
    }

    /*TODO : Maintain a list of frames*/
    Console::puts("Frame pool initialized.\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    assert(_n_frames <= nFreeFrames);
    
    unsigned int available_continuos_free_frames = 0;
    int first_available_free_frame = -1;

    /*Loop around the frames and get the current frame state*/
    for(int fno=0; fno < nframes; fno++){
        ContFramePool::FrameState currentFrameState = get_state(fno);
        if(currentFrameState == FrameState::Free){
            available_continuos_free_frames++;
            if(available_continuos_free_frames == 1){
                first_available_free_frame = fno;
            }
        }else{
            available_continuos_free_frames = 0;
            first_available_free_frame = -1;
        }

        if(available_continuos_free_frames == _n_frames){
            break;
        }
    }

    /*We did not find any available free frames, hence return 0*/
    if(first_available_free_frame == -1){
        return 0;
    }

    /*Determine the last frame*/
    unsigned int last_frame = (first_available_free_frame + _n_frames) - 1;

    /*Set the first frame as head frame*/
    set_state(first_available_free_frame,FrameState::HoS);

    /*Mark all the other continuos frames as used*/
    for(int fno=first_available_free_frame + 1; fno <= last_frame; fno++){
        set_state(fno, FrameState::Used);
    }

    return base_frame_no + first_available_free_frame;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    /*Loop around the frames and mark them as inaccessible in the bitmap*/
    for(int fno=0; fno<_n_frames; fno++){
        set_state(_base_frame_no+fno,FrameState::InA);
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    /*16K is used insted of 32K because, each frame uses 2 bits for status*/
    return _n_frames / (16 * 1024) + (_n_frames % (16 * 1024) > 0 ? 1 : 0);
}
