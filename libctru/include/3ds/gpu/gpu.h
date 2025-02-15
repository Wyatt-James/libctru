/**
 * @file gpu.h
 * @brief Barebones GPU communications driver.
 */
#pragma once

#include "registers.h"
#include "enums.h"

/// Creates a GPU command header from its write increments, mask, and register.
#define GPUCMD_HEADER(incremental, mask, reg) (((incremental)<<31)|(((mask)&0xF)<<16)|((reg)&0x3FF))
#define GPUCMD_UNLIKELY(cond_) __builtin_expect(!!(cond_), 0)
#define GPUCMD_LIKELY(cond_)   __builtin_expect(!!(cond_), 1)
#define GPUCMD_ARRAY_COUNT(arr_) (size_t) (sizeof(arr_) / sizeof(arr_[0]))

typedef struct
{
	u32 param, header;
} gpucmd_single_t;

extern u32* gpuCmdBuf;      ///< GPU command buffer.
extern u32 gpuCmdBufSize;   ///< GPU command buffer size.
extern u32 gpuCmdBufOffset; ///< GPU command buffer offset.

/**
 * @brief Sets the GPU command buffer to use.
 * @param adr Pointer to the command buffer.
 * @param size Size of the command buffer.
 * @param offset Offset of the command buffer.
 */
static inline void GPUCMD_SetBuffer(u32* adr, u32 size, u32 offset)
{
	gpuCmdBuf=adr;
	gpuCmdBufSize=size;
	gpuCmdBufOffset=offset;
}

/**
 * @brief Sets the offset of the GPU command buffer.
 * @param offset Offset of the command buffer.
 */
static inline void GPUCMD_SetBufferOffset(u32 offset)
{
	gpuCmdBufOffset=offset;
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
	if(size)*size=gpuCmdBufSize;
	if(offset)*offset=gpuCmdBufOffset;
}

/**
 * @brief Adds raw GPU commands to the current command buffer.
 * @param cmd Buffer containing commands to add.
 * @param size Size of the buffer.
 */
void GPUCMD_AddRawCommands(const u32* cmd, u32 size);

/**
 * @brief Adds a GPU command to the current command buffer.
 * @param header Header of the command.
 * @param param Parameters of the command.
 * @param paramlength Size of the parameter buffer.
 */
void GPUCMD_Add(u32 header, const u32* param, u32 paramlength);

/**
 * @brief Splits the current GPU command buffer.
 * @param addr Pointer to output the command buffer to.
 * @param size Pointer to output the size (in words) of the command buffer to.
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

/// Adds a command with a single parameter to the current command buffer.
static inline void GPUCMD_AddSingleParam(u32 header, u32 param)
{
	if(GPUCMD_UNLIKELY(!gpuCmdBuf || gpuCmdBufOffset + 2 > gpuCmdBufSize))
		GPUCMD_SvcBreakUserPanicWrapper(); // Shouldn't happen.

	gpuCmdBuf[gpuCmdBufOffset++]=param;
	gpuCmdBuf[gpuCmdBufOffset++]=header;
}

static inline void GPUCMD_AddBatchOfSingles_Int(size_t count, gpucmd_single_t arr[count])
{
	if(GPUCMD_UNLIKELY(!gpuCmdBuf || gpuCmdBufOffset + count * 2 > gpuCmdBufSize))
		GPUCMD_SvcBreakUserPanicWrapper(); // Shouldn't happen.

		for (int i = 0; i < count; i++) {
			gpuCmdBuf[gpuCmdBufOffset + 2 * i + 0] = arr[i].param;
			gpuCmdBuf[gpuCmdBufOffset + 2 * i + 1] = arr[i].header;
		}
	
		gpuCmdBufOffset += count * 2;
}

/// Constructs a masked gpucmd_single_t.
#define GPUCMD_MaskedSingle(reg, mask, val) ((gpucmd_single_t) {.header = GPUCMD_HEADER(0, (mask), (reg)), .param = (val)})
/// Constructs a gpucmd_single_t.
#define GPUCMD_Single(reg, val) GPUCMD_MaskedSingle((reg), 0xF, (val))

/// Adds a batch of gpucmd_single_t commands directly.
#define GPUCMD_AddBatchOfSingles(arr) GPUCMD_AddBatchSingles(GPUCMD_ARRAY_COUNT(arr), arr)
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
