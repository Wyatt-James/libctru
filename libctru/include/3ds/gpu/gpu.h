/**
 * @file gpu.h
 * @brief Barebones GPU communications driver.
 */
#pragma once

#include <string.h>
#include <stdint.h> // uintptr_t

#include "registers.h"
#include "enums.h"

#ifndef GPUCMD_INLINE_THRESH
#define GPUCMD_INLINE_THRESH 6 /* Attempt to inline GPUCMDs <= this count. Configurable. */
#endif

/// Creates a GPU command header from its write increments, mask, and register.
#define GPUCMD_HEADER(incremental, mask, reg) (((incremental)<<31)|(((mask)&0xF)<<16)|((reg)&0x3FF))
#define GPUCMD_UNLIKELY(cond_)                __builtin_expect(!!(cond_), 0)
#define GPUCMD_LIKELY(cond_)                  __builtin_expect(!!(cond_), 1)
#define GPUCMD_IS_CONSTEXPR(expr_)            __builtin_constant_p(expr_)
#define GPUCMD_ARRAY_COUNT(arr_)              (size_t) (sizeof(arr_) / sizeof(arr_[0]))
#define GPUCMD_PACK_U64(top, bottom)          ((((u64) (u32) (top)) << 32) | (u32) (bottom))
#define GPUCMD_HAS_SPACE(num_u32s) 			  (((uintptr_t) gpuCmdBufEnd - (uintptr_t) gpuCmdBufHead) >= ((num_u32s) * sizeof(u32)))

#ifdef GPUCMD_ASSUME_WRITES_ALIGNED
#define GPUCMD_ASSUME_DST_ALIGNED(dst) __builtin_assume_aligned((dst), _Alignof(u64))
#else
#define GPUCMD_ASSUME_DST_ALIGNED(dst) (dst)
#endif


extern u32* gpuCmdBuf;      ///< GPU command buffer.
extern u32* gpuCmdBufHead;  ///< GPU command buffer's next free space.
extern u32* gpuCmdBufEnd;   ///< GPU command buffer + size.

/**
 * @brief Sets the GPU command buffer to use.
 * @param adr Pointer to the command buffer.
 * @param size Size of the command buffer.
 * @param offset Offset of the command buffer.
 */
static inline void GPUCMD_SetBuffer(u32* adr, u32 size, u32 offset)
{
	gpuCmdBuf=adr;
	gpuCmdBufEnd=gpuCmdBuf+size;
	gpuCmdBufHead=gpuCmdBuf+offset;
}

/**
 * @brief Sets the offset of the GPU command buffer.
 * @param offset Offset of the command buffer.
 */
static inline void GPUCMD_SetBufferOffset(u32 offset)
{
	gpuCmdBufHead=gpuCmdBuf+offset;
}

/**
 * @brief Gets the current GPU command buffer.
 * @param addr Pointer to output the command buffer to.
 * @param size Pointer to output the size (in words) of the command buffer to.
 * @param offset Pointer to output the offset of the command buffer to.
 */
static inline void GPUCMD_GetBuffer(u32** addr, u32* size, u32* offset)
{
	if(addr)*addr=gpuCmdBuf;
	if(size)*size=gpuCmdBufEnd-gpuCmdBuf;
	if(offset)*offset=gpuCmdBufHead-gpuCmdBuf;
}

/**
 * @brief Adds raw GPU commands to the current command buffer.
 * @param cmd Buffer containing commands to add.
 * @param size Size of the buffer (in words).
 */
void GPUCMD_AddRawCommands(const u32* cmd, u32 size);

/**
 * @brief Adds a GPU command to the current command buffer.
 * @param header Header of the command.
 * @param param Parameters of the command.
 * @param paramlength Size of the parameter buffer (in words).
 */
void GPUCMD_Add(u32 header, const u32* param, u32 paramlength);

/**
 * @brief Splits the current GPU command buffer.
 * @param addr Pointer to output the command buffer to split.
 * @param size Pointer to output the size (in words) of the command buffer to split.
 */
void GPUCMD_Split(u32** addr, u32* size);

/**
 * @brief Converts a 32-bit float to a 16-bit float.
 * @param f Float to convert.
 * @return The converted float.
 */
u32 f32tof16(float f);

/**
 * @brief Converts a 32-bit float to a 20-bit float.
 * @param f Float to convert.
 * @return The converted float.
 */
u32 f32tof20(float f);

/**
 * @brief Converts a 32-bit float to a 24-bit float.
 * @param f Float to convert.
 * @return The converted float.
 */
u32 f32tof24(float f);

/**
 * @brief Converts a 32-bit float to a 31-bit float.
 * @param f Float to convert.
 * @return The converted float.
 */
u32 f32tof31(float f);

// Wrapper for svcBreak(USERBREAK_PANIC). Used to avoid including 3ds/svc.h in gpu.h.
void GPUCMD_SvcBreakUserPanicWrapper();

/**
 * @brief Adds raw GPU commands to the current command buffer.
 * @param cmd Buffer containing commands to add.
 * @param size Size of the buffer (in words).
 */
static inline void GPUCMD_AddRawCommands_Inline(const u32* cmd, u32 size)
{
	if(GPUCMD_UNLIKELY(!cmd || !size))return; // Not really needed

#ifndef GPUCMD_DISABLE_BOUNDS_CHECKS
	if(GPUCMD_UNLIKELY(!GPUCMD_HAS_SPACE(size))) {
		GPUCMD_SvcBreakUserPanicWrapper(); // Shouldn't happen.
		return;
	}
#endif

	u32* dst = gpuCmdBufHead;
	gpuCmdBufHead += size;
	memcpy(GPUCMD_ASSUME_DST_ALIGNED(dst), cmd, size * sizeof(u32));
}

/// Adds a command with a single parameter to the current command buffer.
static inline void GPUCMD_AddSingleParam(u32 header, u32 param)
{
	u64 cmd[1] = {GPUCMD_PACK_U64(header, param)};
	GPUCMD_AddRawCommands_Inline((u32*) cmd, GPUCMD_ARRAY_COUNT(cmd) * 2);
}

/// Don't use me. Use the macros instead.
static inline void GPUCMD_AddInternal_Inline(u32 header, const u32* param, u32 paramlength)
{
#ifndef GPUCMD_DISABLE_BOUNDS_CHECKS
	if(GPUCMD_UNLIKELY(!GPUCMD_HAS_SPACE(paramlength+1))) {
		GPUCMD_SvcBreakUserPanicWrapper(); // Shouldn't happen.
		return;
	}
#endif

	paramlength--;
	header|=(paramlength&0xff)<<20;

	u32* head = gpuCmdBufHead; // Local copy allows compiler to optimize properly

	*head++=param ? param[0] : 0;
	*head++=header;

#ifdef GPUCMD_ENABLE_ZERO_PADDING
	gpuCmdBufHead = head + paramlength;
	if(paramlength&1) *gpuCmdBufHead++=0x00000000; //alignment
#else
	gpuCmdBufHead = head + paramlength + (paramlength & 1); // Add LSB twice for alignment
#endif

	if(GPUCMD_LIKELY(paramlength))
	{
		// The alignment of param isn't easily solvable. Ideally, param[0] would be
		// 32-bit aligned but NOT 64-bit aligned such that param[1] is 64-bit aligned,
		// but that requires intelligent allocation.
		if(GPUCMD_LIKELY(param))
			memcpy(GPUCMD_ASSUME_DST_ALIGNED(head), param + 1, paramlength * sizeof(u32));
		else
			memset(GPUCMD_ASSUME_DST_ALIGNED(head), 0, paramlength * sizeof(u32));
	}
}

/// Don't use me. Use the macros instead.
static inline void GPUCMD_Add_Inline(u32 header, const u32* param, u32 paramlength)
{
	if(!paramlength)paramlength=1;

	do {
		u32 remaining = paramlength > 0x100 ? 0x100 : paramlength;
		GPUCMD_AddInternal_Inline(header, param, remaining);
		param += remaining;
		paramlength -= remaining;
		if(header & BIT(31)) header += remaining;
	} while(GPUCMD_UNLIKELY(paramlength));
}

/// Constructs a masked GPUCMD
#define GPUCMD_MaskedSingle(reg, mask, val) (GPUCMD_PACK_U64(GPUCMD_HEADER(0, (mask), (reg)), (val)))
/// Constructs a GPUCMD
#define GPUCMD_Single(reg, val) GPUCMD_MaskedSingle((reg), 0xF, (val))

/// Adds a batch of u64 GPUCMD commands directly. WYATT_TODO improve naming because singles are just u64s now.
#define GPUCMD_AddBatchOfSingles(arr) GPUCMD_AddRawCommands_Inline((u32*) arr, GPUCMD_ARRAY_COUNT(arr) * 2)
/// Adds a masked register write to the current command buffer.
#define GPUCMD_AddMaskedWrite(reg, mask, val) GPUCMD_AddSingleParam(GPUCMD_HEADER(0, (mask), (reg)), (val))
/// Adds a register write to the current command buffer.
#define GPUCMD_AddWrite(reg, val) GPUCMD_AddMaskedWrite((reg), 0xF, (val))
/// Adds multiple masked register writes to the current command buffer.
#define GPUCMD_AddMaskedWrites(reg, mask, vals, num) GPUCMD_Add(GPUCMD_HEADER(0, (mask), (reg)), (vals), (num))
/// Adds multiple register writes to the current command buffer.
#define GPUCMD_AddWrites(reg, vals, num) GPUCMD_AddMaskedWrites((reg), 0xF, (vals), (num))
/// Adds multiple masked incremental register writes to the current command buffer.
#define GPUCMD_AddMaskedIncrementalWrites(reg, mask, vals, num) GPUCMD_Add(GPUCMD_HEADER(1, (mask), (reg)), (vals), (num))
/// Adds multiple incremental register writes to the current command buffer.
#define GPUCMD_AddIncrementalWrites(reg, vals, num) GPUCMD_AddMaskedIncrementalWrites((reg), 0xF, (vals), (num))

/// Macros that always inline multiple writes

/// Adds multiple masked register writes to the current command buffer. This will always inline.
#define GPUCMD_AddMaskedWrites_Inline(reg, mask, vals, num) GPUCMD_Add_Inline(GPUCMD_HEADER(0, (mask), (reg)), (vals), (num))
/// Adds multiple register writes to the current command buffer. This will always inline.
#define GPUCMD_AddWrites_Inline(reg, vals, num) GPUCMD_AddMaskedWrites_Inline((reg), 0xF, (vals), (num))
/// Adds multiple masked incremental register writes to the current command buffer. This will always inline.
#define GPUCMD_AddMaskedIncrementalWrites_Inline(reg, mask, vals, num) GPUCMD_Add_Inline(GPUCMD_HEADER(1, (mask), (reg)), (vals), (num))
/// Adds multiple incremental register writes to the current command buffer. This will always inline.
#define GPUCMD_AddIncrementalWrites_Inline(reg, vals, num) GPUCMD_AddMaskedIncrementalWrites_Inline((reg), 0xF, (vals), (num))

/// Macros that inline automatically

/// Adds multiple masked register writes to the current command buffer.
/// This "auto" macro will attempt to automatically inline calls
/// where "num" is a constant expression, and also <= a threshold.
#define GPUCMD_AddMaskedWrites_Auto(reg, mask, vals, num)				\
do {																	\
	if (GPUCMD_IS_CONSTEXPR(num) && (num) <= GPUCMD_INLINE_THRESH)		\
		GPUCMD_AddMaskedWrites_Inline((reg), (mask), (vals), (num));	\
	else																\
		GPUCMD_AddMaskedWrites((reg), (mask), (vals), (num));			\
} while (0)

/// Adds multiple register writes to the current command buffer.
/// This "auto" macro will attempt to automatically inline calls
/// where "num" is a constant expression, and also <= a threshold.
#define GPUCMD_AddWrites_Auto(reg, vals, num) GPUCMD_AddMaskedWrites_Auto((reg), 0xF, (vals), (num))

/// Adds multiple masked incremental register writes to the current command buffer.
/// This "auto" macro will attempt to automatically inline calls
/// where "num" is a constant expression, and also <= a threshold.
#define GPUCMD_AddMaskedIncrementalWrites_Auto(reg, mask, vals, num)			\
do {																			\
	if (GPUCMD_IS_CONSTEXPR(num) && (num) <= GPUCMD_INLINE_THRESH)				\
		GPUCMD_AddMaskedIncrementalWrites_Inline((reg), (mask), (vals), (num));	\
	else																		\
		GPUCMD_AddMaskedIncrementalWrites((reg), (mask), (vals), (num));		\
} while(0)

/// Adds multiple incremental register writes to the current command buffer.
/// This "auto" macro will attempt to automatically inline calls
/// where "num" is a constant expression, and also <= a threshold.
#define GPUCMD_AddIncrementalWrites_Auto(reg, vals, num) GPUCMD_AddMaskedIncrementalWrites_Auto((reg), 0xF, (vals), (num))
