#include "Arm.h"
#include "../Memory/MemoryBus.h"

Arm::Arm(MemoryBus *mbus)
	:addrMode1(*this), addrMode2(*this),
	addrMode3(*this), addrMode4(*this)
{
	mapArmOpcodes();
	mapThumbOpcodes();

	this->mbus = mbus;
}

u8 Arm::clock()
{
	if (state == State::ARM) {
		currentExecutingArmOpcode = armpipeline[0];
		armpipeline[0] = armpipeline[1];
		
		ArmInstruction ins;
		ins.encoding = currentExecutingArmOpcode;
		executeArmIns(ins);

		armpipeline[1] = fetchU32(); 
	}
	else if (state == State::THUMB) {
		currentExecutingThumbOpcode = thumbpipeline[0];
		thumbpipeline[0] = thumbpipeline[1];
		
		ThumbInstruction ins;
		ins.encoding = currentExecutingThumbOpcode;
		executeThumbIns(ins);

		thumbpipeline[1] = fetchU16();
	}
	checkStateAndProcessorMode();

	return cycles;
}

void Arm::checkStateAndProcessorMode()
{
	mode = getProcessorMode();
	state = getState();
}

void Arm::reset()
{
	cycles = 0;
	for (s32 i = 0; i < NUM_REGISTERS; i++) registers[i].value = 0x0;
	for (s32 i = 0; i < NUM_REGISTERS_FIQ; i++) registersFiq[i].value = 0x0;
	
	//System/User
	LR = 0x00000000; //R14
	R15 = 0x08000000;
	SP = 0x03007F00; //R13
	CPSR = 0x000000DF;
	SPSR = 0x000000DF;
	SPSR_irq = 0x0;

	SP_irq = 0x03007FA0;
	SP_svc = 0x03007FE0;

	mode = getProcessorMode();

	armpipeline[0] = fetchU32();
	armpipeline[1] = fetchU32();
}

void Arm::setFlag(u32 flagBits, bool condition)
{
	if (condition) {
		setFlag(flagBits);
	}
}

void Arm::setFlag(u32 flagBits)
{
	//M0 - M4
	for (s32 i = 0; i < 5; i++) {
		u8 mi = (1 << i);
		if (flagBits & mi) {
			CPSR |= mi;
		}
	}

	if (flagBits & T) {
		CPSR |= T;
	}
	if (flagBits & F) {
		CPSR |= F;
	}
	if (flagBits & I) {
		CPSR |= I;
	}
	if (flagBits & V) {
		CPSR |= V;
	}
	if (flagBits & C) {
		CPSR |= C;
	}
	if (flagBits & Z) {
		CPSR |= Z;
	}
	if (flagBits & N) {
		CPSR |= N;
	}
}

void Arm::clearFlag(u32 flagBits)
{
	//M0 - M4
	for (s32 i = 0; i < 5; i++) {
		u8 mi = (1 << i);
		if (flagBits & mi) {
			CPSR &= ~(mi);
		}
	}

	if (flagBits & T) {
		CPSR &= ~(T);
	}
	if (flagBits & F) {
		CPSR &= ~(F);
	}
	if (flagBits & I) {
		CPSR &= ~(I);
	}
	if (flagBits & V) {
		CPSR &= ~(V);
	}
	if (flagBits & C) {
		CPSR &= ~(C);
	}
	if (flagBits & Z) {
		CPSR &= ~(Z);
	}
	if (flagBits & N) {
		CPSR &= ~(N);
	}
}

void Arm::setFlagSPSR(u32 flagBits)
{
	//M0 - M4
	for (s32 i = 0; i < 5; i++) {
		u8 mi = (1 << i);
		if (flagBits & mi) {
			SPSR |= mi;
		}
	}

	if (flagBits & T) {
		SPSR |= T;
	}
	if (flagBits & F) {
		SPSR |= F;
	}
	if (flagBits & I) {
		SPSR |= I;
	}
	if (flagBits & V) {
		SPSR |= V;
	}
	if (flagBits & C) {
		SPSR |= C;
	}
	if (flagBits & Z) {
		SPSR |= Z;
	}
	if (flagBits & N) {
		SPSR |= N;
	}
}

void Arm::clearFlagSPSR(u32 flagBits)
{
	//M0 - M4
	for (s32 i = 0; i < 5; i++) {
		u8 mi = (1 << i);
		if (flagBits & mi) {
			SPSR &= ~(mi);
		}
	}

	if (flagBits & T) {
		SPSR &= ~(T);
	}
	if (flagBits & F) {
		SPSR &= ~(F);
	}
	if (flagBits & I) {
		SPSR &= ~(I);
	}
	if (flagBits & V) {
		SPSR &= ~(V);
	}
	if (flagBits & C) {
		SPSR &= ~(C);
	}
	if (flagBits & Z) {
		SPSR &= ~(Z);
	}
	if (flagBits & N) {
		SPSR &= ~(N);
	}
}

void Arm::fillPipeline()
{
	u32 first_instr = (mbus->readU8(R15)) | (mbus->readU8(R15 + 1) << 8) |
		(mbus->readU8(R15 + 2) << 16) | (mbus->readU8(R15 + 3) << 24);

	u32 second_instr = (mbus->readU8(R15 + 4)) | (mbus->readU8(R15 + 5) << 8) |
		(mbus->readU8(R15 + 6) << 16) | (mbus->readU8(R15 + 7) << 24);

	//Fill pipeline at boot with first 2 instructions
	armpipeline[0] = first_instr;
	armpipeline[1] = second_instr;
}

void Arm::flushPipeline() 
{
	armpipeline[0] = fetchU32();
	armpipeline[1] = readU32();
}

void Arm::flushThumbPipeline()
{
	thumbpipeline[0] = fetchU16();
	thumbpipeline[1] = readU16();
}

void Arm::enterSupervisorMode()
{
	clearFlag(M2 | M3);
	setFlag(M0 | M1 | M4);
}

u8 Arm::getFlag(u32 flag)
{
	return (CPSR & flag) ? 1 : 0;
}

u8 Arm::carryFrom(u32 op1, u32 op2)
{
	return (((u64)op1 + (u64)op2) > 0xFFFFFFFF);
}

u8 Arm::borrowFrom(u32 op1, u32 op2)
{
	return (((s64)op1 - (s64)op2) < 0);
}

u8 Arm::overflowFromAdd(u32 op1, u32 op2)
{
	u32 signBit = 0x80000000;
	bool negative = ((op1 & signBit) && (op2 & signBit));
	bool positive = (((op1 & signBit) == 0) && ((op2 & signBit) == 0));

	if (positive) {
		s64 result = (s64)op1 + (s64)op2;
		if (result & signBit) return true;
	}
	if (negative) {
		s64 result = (s64)op1 + (s64)op2;
		if ((result & signBit) == 0) return true;
	}
	return false;
}

u8 Arm::overflowFromSub(u32 op1, u32 op2) 
{
	u32 signBit = 0x80000000;
	//subtracting a negative (5 - (-2))
	bool negative = (((op1 & signBit) == 0) && (op2 & signBit));
	//subtracting a positive (-5 - 2)
	bool positive = ((op1 & signBit) && ((op2 & signBit) == 0));

	if (positive) {
		s64 result = op1 - op2;
		if (result & signBit) return true;
	}
	if (negative) {
		s64 result = op1 - op2;
		if ((result & signBit) == 0) return true;
	}
	return false;
}

u8 Arm::fetchOp(u32 encoding)
{
	u8 opcode = ((encoding >> 21) & 0xF);
	return opcode;
}

PSR Arm::getPSR()
{
	return PSR{
		CPSR,
		SPSR
	};
}

u32 Arm::getSPSR()
{
	if (mode == ProcessorMode::USER || mode == ProcessorMode::SYS) {
		return CPSR;
	}
	else if (mode == ProcessorMode::FIQ) {
		return SPSR_fiq;
	}
	else if (mode == ProcessorMode::IRQ) {
		return SPSR_irq;
	}
	else if (mode == ProcessorMode::SVC) {
		return SPSR_svc;
	}
	else if (mode == ProcessorMode::ABT) {
		return SPSR_abt;
	}
	else if (mode == ProcessorMode::UND) {
		return SPSR_und;
	}
}

u32 Arm::getRegister(RegisterID reg)
{
	//Registers r0 - r7 remain the same for all states
	if (reg.id <= 7) {
		return registers[reg.id].value;
	}
	else {
		if (mode == ProcessorMode::USER || mode == ProcessorMode::SYS) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP;
				if (reg.id == R14_ID) return LR;
				if (reg.id == R15_ID) return R15;
			}
			else {
				//return registers within r8 - r12 range
				return registers[reg.id].value;
			}
		}
		else if (mode == ProcessorMode::FIQ) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP_fiq;
				if (reg.id == R14_ID) return LR_fiq;
				if (reg.id == R15_ID) return R15;
			}
			else {
				u8 index = reg.id - 8;
				if (index == NUM_REGISTERS_FIQ) index -= 1; //array out of bounds check
				return registersFiq[index].value;
			}
		}
		else if (mode == ProcessorMode::IRQ) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP_irq;
				if (reg.id == R14_ID) return LR_irq;
				if (reg.id == R15_ID) return R15;
			}
			else {
				return registers[reg.id].value;
			}
		}
		else if (mode == ProcessorMode::SVC) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP_svc;
				if (reg.id == R14_ID) return LR_svc;
				if (reg.id == R15_ID) return R15;
			}
			else {
				return registers[reg.id].value;
			}
		}
		else if (mode == ProcessorMode::ABT) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP_abt;
				if (reg.id == R14_ID) return LR_abt;
				if (reg.id == R15_ID) return R15;
			}
			else {
				return registers[reg.id].value;
			}
		}
		else if (mode == ProcessorMode::UND) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) return SP_und;
				if (reg.id == R14_ID) return LR_und;
				if (reg.id == R15_ID) return R15;
			}
			else {
				return registers[reg.id].value;
			}
		}
	}
}

void Arm::writeRegister(RegisterID reg, u32 value)
{
	//Registers r0 - r7 remain the same for all states
	if (reg.id <= 7) {
		registers[reg.id].value = value;
	}
	else {
		if (mode == ProcessorMode::USER || mode == ProcessorMode::SYS) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP = value;
				if (reg.id == R14_ID) LR = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				//modify registers within r8 - r12 range
				registers[reg.id].value = value;
			}
		}
		else if (mode == ProcessorMode::FIQ) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP_fiq = value;;
				if (reg.id == R14_ID) LR_fiq = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				u8 index = reg.id - 8;
				if (index == NUM_REGISTERS_FIQ) index -= 1;
				registersFiq[index].value = value;
			}
		}
		else if (mode == ProcessorMode::IRQ) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP_irq = value;
				if (reg.id == R14_ID) LR_irq = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				registers[reg.id].value = value;
			}
		}
		else if (mode == ProcessorMode::SVC) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP_svc = value;
				if (reg.id == R14_ID) LR_svc = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				registers[reg.id].value = value;
			}
		}
		else if (mode == ProcessorMode::ABT) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP_abt = value;
				if (reg.id == R14_ID) LR_abt = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				registers[reg.id].value = value;
			}
		}
		else if (mode == ProcessorMode::UND) {
			if (reg.id >= R13_ID) {
				if (reg.id == R13_ID) SP_und = value;
				if (reg.id == R14_ID) LR_und = value;
				if (reg.id == R15_ID) R15 = value;
			}
			else {
				registers[reg.id].value = value;
			}
		}
	}
}

void Arm::writePC(u32 pc)
{
	R15 |= ((pc << 2) & 0xFFFFFFFF);
}

void Arm::setCC(u32 rd, bool borrow, bool overflow,
	bool shiftOut, u8 shifterCarryOut)
{
	if (rd != R15) {
		(rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
		(rd == 0) ? setFlag(Z) : clearFlag(Z);
		if (shiftOut) {
			//overflow not affected
			(shifterCarryOut == 1) ? setFlag(C) : clearFlag(C);
		}
		else {
			(borrow == false) ? setFlag(C) : clearFlag(C);
			(overflow == true) ? setFlag(V) : clearFlag(V);
		}
	}
	else {
		CPSR = SPSR;
	}
}

ProcessorMode Arm::getProcessorMode()
{
	ProcessorMode mode = ProcessorMode::SYS;
	u8 mbits = CPSR & 0x1F;
	switch (mbits) {
		case 0b10000: mode = ProcessorMode::USER; break;
		case 0b10001: mode = ProcessorMode::FIQ; break;
		case 0b10010: mode = ProcessorMode::IRQ; break;
		case 0b10011: mode = ProcessorMode::SVC; break;
		case 0b10111: mode = ProcessorMode::ABT; break;
		case 0b11011: mode = ProcessorMode::UND; break;
		case 0b11111: mode = ProcessorMode::SYS; break;
	}
	return mode;
}

State Arm::getState()
{
	State cs = (State)getFlag(T);
	return cs;
}

u8 Arm::getConditionCode(u8 cond)
{
	switch (cond) {
		case 0b0000: return getFlag(Z); break;
		case 0b0001: return (getFlag(Z) == 0); break;
		case 0b0010: return getFlag(C); break;
		case 0b0011: return (getFlag(C) == 0); break;
		case 0b0100: return getFlag(N); break;
		case 0b0101: return (getFlag(N) == 0); break;
		case 0b0110: return getFlag(V); break;
		case 0b0111: return (getFlag(V) == 0); break;
		case 0b1000: return (getFlag(C) && (getFlag(Z) == 0)); break;
		case 0b1001: return ((getFlag(C) == 0) || getFlag(Z)); break;
		case 0b1010: return (getFlag(N) == getFlag(V)); break;
		case 0b1011: return (getFlag(N) != getFlag(V)); break;
		case 0b1100: return ((getFlag(Z) == 0) && (getFlag(N) == getFlag(V))); break;
		case 0b1101: return (getFlag(Z) || (getFlag(N) != getFlag(V))); break;
		case 0b1110: return 1; break;//always
		default: return 1; break; //always
	}
}

u16 Arm::readU16()
{
	u16 halfword = (mbus->readU8(R15)) |
		(mbus->readU8(R15 + 1) << 8);

	return halfword;
}

u16 Arm::fetchU16()
{
	u16 halfword = (mbus->readU8(R15++)) |
		(mbus->readU8(R15++) << 8);

	return halfword;
}

u32 Arm::readU32()
{
	u32 word = (mbus->readU8(R15)) |
		(mbus->readU8(R15 + 1) << 8) |
		(mbus->readU8(R15 + 2) << 16) |
		(mbus->readU8(R15 + 3) << 24);

	return word;
}

u32 Arm::fetchU32()
{
	u32 word = (mbus->readU8(R15++)) |
		(mbus->readU8(R15++) << 8) |
		(mbus->readU8(R15++) << 16) |
		(mbus->readU8(R15++) << 24);

	return word;
}

u32 Arm::shift(u32 value, u8 amount, u8 type, u8 &shiftedBit)
{
	switch (type) {
		case 0b00:
			value = lsl(value, amount, shiftedBit);
			break;

		case 0b01:
			value = lsr(value, amount, shiftedBit);
			break;

		case 0b10:
			value = asr(value, amount, shiftedBit);
			break;

		case 0b11: {
			if (amount == 0) {
				value = rrx(value, shiftedBit);
				return value;
			}
			value = ror(value, amount);
			//Save last carried out bit
			shiftedBit = (value >> 31) & 0x1;
		}
			break;
	}
	return value;
}

u32 Arm::lsl(u32 value, u8 shift, u8& shiftedBit)
{
	//Save last carried out bit
	shiftedBit = (31 - (shift - 1));
	shiftedBit = ((value >> shiftedBit) & 0x1);

	value <<= shift;
	return value;
}

u32 Arm::lsr(u32 value, u8 shift, u8& shiftedBit)
{
	shiftedBit = (shift - 1);
	shiftedBit = ((value >> shiftedBit) & 0x1);

	value >>= shift;
	return value;
}

u32 Arm::asr(u32 value, u8 shift, u8& shiftedBit)
{
	shiftedBit = (shift - 1);
	shiftedBit = ((value >> shiftedBit) & 0x1);

	u8 msb = ((value >> 31) & 0x1);
	value >>= shift;
	value |= (msb << 31);

	return value;
}

u32 Arm::ror(u32 value, u8 shift)
{
	u32 rotatedOut = getNthBits(value, 0, shift);
	u32 rotatedIn = getNthBits(value, shift, 31);

	u32 result = ((rotatedIn) | (rotatedOut << (32 - shift)));
	return result;
}

u32 Arm::rrx(u32 value, u8 &shiftedBit)
{
	//shifter_carry_out
	shiftedBit = value & 0x1;
	value = (((u32)getFlag(C) << 31) | (value >> 1));
	
	return value;
}

void Arm::executeArmIns(ArmInstruction& ins)
{
	u8 cond = getConditionCode(ins.cond());
	if (!cond) {
		cycles = 1;
		return;
	}
	
	u16 instruction = ins.instruction();
	cycles = armlut[instruction](ins);
}

void Arm::executeThumbIns(ThumbInstruction& ins)
{
	u8 instruction = ins.instruction();
	cycles = thumblut[instruction](ins);
}

u8 Arm::executeDataProcessing(ArmInstruction& ins, bool flags, bool immediate)
{
	RegisterID rd = ins.rd();
	RegisterID rn = ins.rn();

	switch (ins.opcode()) {
		case 0b0000: return opAND(ins, rd, rn, flags, immediate); break;
		case 0b0001: return opEOR(ins, rd, rn, flags, immediate); break;
		case 0b0010: return opSUB(ins, rd, rn, flags, immediate); break;
		case 0b0011: return opRSB(ins, rd, rn, flags, immediate); break;
		case 0b0100: return opADD(ins, rd, rn, flags, immediate); break;
		case 0b0101: return opADC(ins, rd, rn, flags, immediate); break;
		case 0b0110: return opSBC(ins, rd, rn, flags, immediate); break;
		case 0b0111: return opRSC(ins, rd, rn, flags, immediate); break;
		case 0b1000: return opTST(ins, rd, rn, flags, immediate); break;
		case 0b1001: return opTEQ(ins, rd, rn, flags, immediate); break;
		case 0b1010: return opCMP(ins, rd, rn, flags, immediate); break;
		case 0b1011: return opCMN(ins, rd, rn, flags, immediate); break;
		case 0b1100: return opORR(ins, rd, rn, flags, immediate); break;
		case 0b1101: return opMOV(ins, rd, rn, flags, immediate); break;
		case 0b1110: return opBIC(ins, rd, rn, flags, immediate); break;
		case 0b1111: return opMVN(ins, rd, rn, flags, immediate); break;
	}

	return 1;
}

u8 Arm::executeDataProcessingImmFlags(ArmInstruction& ins)
{
	executeDataProcessing(ins, true, true);
	return 1;
}

u8 Arm::executeDataProcessingImm(ArmInstruction& ins)
{
	executeDataProcessing(ins, false, true);
	return 1;
}

u8 Arm::executeDataProcessingImmShiftFlags(ArmInstruction& ins)
{
	executeDataProcessing(ins, true, false);
	return 1;
}

u8 Arm::executeDataProcessingImmShift(ArmInstruction& ins)
{
	executeDataProcessing(ins, false, false);
	return 1;
}

u8 Arm::executeDataProcessingRegShiftFlags(ArmInstruction& ins)
{
	executeDataProcessing(ins, true, false);
	return 1;
}

u8 Arm::executeDataProcessingRegShift(ArmInstruction& ins)
{
	executeDataProcessing(ins, false, false);
	return 1;
}

u8 Arm::handleUndefinedIns(ArmInstruction& ins)
{
	u16 lutIndex = ins.instruction();
	printf("LUT Index: %d ", lutIndex);
	printf("ARM undefined or unimplemented instruction 0x%08X at PC: 0x%08X\n", ins.encoding, R15 - 8);

	return 0;
}

u8 Arm::executeLoadStore(ArmInstruction& ins, AddrModeLoadStoreResult& result)
{
	RegisterID rn = ins.rn();
	RegisterID rd = ins.rd();

	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	bool byte = (ins.B() == 0x1); //true == byte, false == word (transfer quantity)
	bool load = (ins.L() == 0x1); //load or store

	switch (result.type) {
		case AddrModeLoadStoreType::PREINDEXED:
			reg_rn = result.address;
			writeRegister(rn, reg_rn);
			break;

		case AddrModeLoadStoreType::POSTINDEX:
			reg_rn = result.rn;
			writeRegister(rn, reg_rn);
			break;
	}

	bool useImmediateOffset = (result.type == AddrModeLoadStoreType::OFFSET);
	u32 address = result.address; //imm offset (no writeback to register rn)

	if (load) {
		//LDR, LDRB
		if (byte) {
			(useImmediateOffset == true) ? opLDRB(ins, rd, address) : opLDRB(ins, rd, reg_rn);
		}
		else {
			(useImmediateOffset == true) ? opLDR(ins, rd, address) : opLDR(ins, rd, reg_rn);
		}
	}
	else {
		//STR, STRB
		if (byte) {
			(useImmediateOffset == true) ? opSTRB(ins, rd, address) : opSTRB(ins, rd, reg_rn);
		}
		else {
			(useImmediateOffset == true) ? opSTR(ins, rd, address) : opSTR(ins, rd, reg_rn);
		}
	}


	return 1;
}

u8 Arm::executeLoadStoreImm(ArmInstruction& ins)
{
	AddrModeLoadStoreResult result = addrMode2.immOffsetIndex(ins);
	executeLoadStore(ins, result);

	return 1;
}

u8 Arm::executeLoadStoreShift(ArmInstruction& ins)
{
	AddrModeLoadStoreResult result = addrMode2.scaledRegisterOffsetIndex(ins);
	executeLoadStore(ins, result);

	return 1;
}

u8 Arm::executeMiscLoadAndStore(ArmInstruction& ins, AddrModeLoadStoreResult &result)
{
	RegisterID rn = ins.rn();
	RegisterID rd = ins.rd();

	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	bool load = (ins.L() == 0x1);
	u8 SH = (ins.S() << 1) | ins.H();

	switch (result.type) {
		case AddrModeLoadStoreType::PREINDEXED:
			reg_rn = result.address;
			writeRegister(rn, reg_rn);
			break;
		
		case AddrModeLoadStoreType::POSTINDEX:
			reg_rn = result.rn;
			writeRegister(rn, reg_rn);
			break;
	}

	bool useImmediateOffset = (result.type == AddrModeLoadStoreType::OFFSET);
	u32 address = result.address; //imm offset (no writeback to register rn)
	
	switch (SH) {
		case 0b00: /*SWP*/ break;
		case 0b01: //Unsigned halfwords
		{
			if (!load) {
				(useImmediateOffset == true) ? opSTRH(ins, rd, address) 
					: opSTRH(ins, rd, reg_rn); 
			}
			else {
				(useImmediateOffset == true) ? opLDRH(ins, rd, address) 
					: opLDRH(ins, rd, reg_rn); 
			}
		}
		break;

		case 0b10: //Signed byte
		{
			if (!load) {
				(useImmediateOffset == true) ? opSTRH(ins, rd, address)
					: opSTRH(ins, rd, reg_rn);
			}
			else { 
				(useImmediateOffset == true) ? opLDRSB(ins, rd, address) 
					: opLDRSB(ins, rd, reg_rn); 
			}
		}
		break;

		case 0b11: //Signed halfword
		{
			if (!load) {
				(useImmediateOffset == true) ? opSTRH(ins, rd, address)
					: opSTRH(ins, rd, reg_rn);
			}
			else {
				(useImmediateOffset == true) ? opLDRSH(ins, rd, address) 
					: opLDRSH(ins, rd, reg_rn); 
			}
		}
		break;
	}

	return 1;
}

u8 Arm::executeMiscLoadStoreImm(ArmInstruction& ins)
{
	AddrModeLoadStoreResult result = addrMode3.immOffsetIndex(ins);
	executeMiscLoadAndStore(ins, result);

	return 1;
}

u8 Arm::executeMiscLoadStoreReg(ArmInstruction& ins)
{
	AddrModeLoadStoreResult result = addrMode3.registerOffsetIndex(ins);
	executeMiscLoadAndStore(ins, result);

	return 1;
}

u8 Arm::executeLDM(ArmInstruction& ins)
{
	u8 PU = (ins.P() << 1) | ins.U();
	u8 S = ins.S_bit22();
	AddrMode4Result result;
	switch (PU) {
		case 0b01: result = addrMode4.incrementAfter(ins); break;
		case 0b11: result = addrMode4.incrementBefore(ins); break;
		case 0b00: result = addrMode4.decrementAfter(ins); break;
		case 0b10: result = addrMode4.decrementBefore(ins); break;
	}

	u16 reg_list = ins.registerList();
	u32 addr = result.startAddress;
	u32 end = result.endAddress;

	for (s32 i = 0; i <= 14; i++) {
		bool included = testBit(reg_list, i);
		if (included) {
			RegisterID id; id.id = i;

			u32 value = mbus->readU32(addr);
			writeRegister(id, value);
			addr += 4;
		}

		if (end == addr - 4) break;
	}

	//R15
	if (testBit(reg_list, 15)) {
		u32 value = mbus->readU32(addr);
		R15 = value & 0xFFFFFFFC;
		flushPipeline();

		if (S == 0x1) {
			CPSR = getSPSR();
		}
	}

	if (result.writeback) {
		RegisterID rn = ins.rn();
		writeRegister(rn, result.rn);
	}

	return 1;
}

u8 Arm::executeSTM(ArmInstruction& ins)
{
	u8 PU = (ins.P() << 1) | ins.U();
	AddrMode4Result result;
	switch (PU) {
	case 0b01: result = addrMode4.incrementAfter(ins); break;
	case 0b11: result = addrMode4.incrementBefore(ins); break;
	case 0b00: result = addrMode4.decrementAfter(ins); break;
	case 0b10: result = addrMode4.decrementBefore(ins); break;
	}

	u16 reg_list = ins.registerList();
	u32 addr = result.startAddress;
	u32 end = result.endAddress;

	for (s32 i = 0; i <= 15; i++) {
		bool included = testBit(reg_list, i);
		if (included) {
			RegisterID id; id.id = i;

			u32 reg = getRegister(id);
			mbus->writeU32(addr, reg);
			addr += 4;
		}

		if (end == addr - 4) break;
	}

	if (result.writeback) {
		RegisterID rn = ins.rn();
		writeRegister(rn, result.rn);
	}

	return 1;
}

u8 Arm::executeMSRImm(ArmInstruction& ins)
{
	u8 immediate = ins.imm();
	u8 rotate_imm = ins.rotate();
	u32 operand = ror(immediate, (rotate_imm * 2));

	opMSR(ins, operand);

	return 1;
}

u8 Arm::executeMSRReg(ArmInstruction& ins)
{
	RegisterID rm = ins.rm();
	u32 operand = getRegister(rm);
	opMSR(ins, operand);

	return 1;
}

u8 Arm::executeThumbUnconditionalBranch(ThumbInstruction& ins)
{
	u8 H = ins.H();
	if (H == 0b00)
		thumbOpB(ins);
	else
		thumbOpBL(ins);

	return 1;
}

u8 Arm::executeThumbLoadFromPool(ThumbInstruction& ins)
{
	thumbOpLDRPool(ins);

	return 1;
}

u8 Arm::executeThumbLoadStoreRegisterOffset(ThumbInstruction& ins)
{
	u8 opcode = ins.opcode3();
	switch (opcode) {
		case 0b100: 
			thumbOpLDR(ins);
			break;
	}

	return 1;
}

u8 Arm::executeThumbShiftByImm(ThumbInstruction& ins)
{
	u8 opcode = ins.opcode2();
	switch (opcode) {
		case 0b00: thumbOpLSL(ins, ins.imm5()); break;
		case 0b01: thumbOpLSR(ins, ins.imm5()); break;
		case 0b10: thumbOpASR(ins, ins.imm5()); break;
	}

	return 1;
}

u8 Arm::executeThumbDataProcessingImm(ThumbInstruction& ins)
{
	u8 opcode = ins.opcode2();
	switch (opcode) {
		case 0b00: thumbOpMOV(ins, ins.imm8()); break;
		case 0b01: thumbOpCMP(ins, ins.imm8()); break;
		case 0b10: thumbOpADD(ins, ins.rdUpper(), ins.imm8()); break;
		case 0b11: thumbOpSUB(ins, ins.rdUpper(), ins.imm8()); break;
	}

	return 1;
}

u8 Arm::executeThumbDataProcessingReg(ThumbInstruction& ins)
{
	u8 opcode = ins.opcode5();

	RegisterID rm = ins.rmLower();
	RegisterID rd = ins.rdLower();
	RegisterID rn = ins.rnLower();
	RegisterID rs = ins.rs();

	switch (opcode) {
		case 0b0000: thumbOpAND(ins, rm, rd); break;
		case 0b0001: thumbOpEOR(ins, rm, rd); break;
		case 0b0010: thumbOpLSL(ins, rs, rd); break;
		case 0b0100: thumbOpASR(ins, rs, rd); break;
		case 0b0101: thumbOpADC(ins, rm, rd); break;
		case 0b1001: thumbOpNEG(ins, rm, rd); break;
		case 0b1010: thumbOpCMP(ins, rm, rn, false); break;
		case 0b1011: thumbOpCMN(ins, rm, rn); break;
		case 0b1101: thumbOpMUL(ins, rm, rd); break;
		case 0b1110: thumbOpBIC(ins, rm, rd); break;
		case 0b1111: thumbOpMVN(ins, rm, rd); break;
	}

	return 1;
}

u8 Arm::executeThumbAddSubReg(ThumbInstruction& ins)
{
	u8 opc = ins.opc();
	RegisterID rm = ins.rmUpper();
	RegisterID rn = ins.rnMiddle();
	RegisterID rd = ins.rdLower();
	if (opc == 0x0) {
		thumbOpADD(ins, rm, rn, rd);
	}
	else {
		thumbOpSUB(ins, rm, rn, rd);
	}

	return 1;
}

u8 Arm::executeThumbAddSubImm(ThumbInstruction& ins)
{
	u8 opc = ins.opc();
	RegisterID rn = ins.rnMiddle();
	RegisterID rd = ins.rdLower();
	u8 imm3 = ins.imm3();

	if (opc == 0x0) {
		thumbOpADD(ins, rn, rd, imm3);
	}
	else {
		thumbOpSUB(ins, rn, rd, imm3);
	}

	return 1;
}

u8 Arm::executeThumbLoadStoreMultiple(ThumbInstruction& ins)
{
	u8 L = ins.L();
	if (L == 0x0) {
		thumbOpSTMIA(ins);
	}
	else {
		thumbOpLDMIA(ins);
	}

	return 1;
}

u8 Arm::executeThumbBranchExchange(ThumbInstruction& ins)
{
	u8 L = (ins.encoding >> 7) & 0x1;
	if (L == 0x0) {
		thumbOpBX(ins);
	}

	return 1;
}

u8 Arm::executeThumbSpecialDataProcessing(ThumbInstruction& ins)
{
	//Hi registers
	u8 opcode = ins.opcode();

	RegisterID rm = ins.rmLower();
	RegisterID rn = ins.rnLower();
	RegisterID rd = ins.rdLower();

	switch (opcode) {
		case 0b00: thumbOpADD(ins, rm, rd); break;
		case 0b01: thumbOpCMP(ins, rm, rn, true); break;
		case 0b10: thumbOpMOV(ins, rm, rd); break;
	}

	return 1;
}

u8 Arm::executeThumbAddSPOrPC(ThumbInstruction& ins)
{
	u8 reg = ins.reg();
	RegisterID rd = ins.rdUpper();
	if (reg == 0x0)
		thumbOpADD(ins, rd, true);
	else
		thumbOpADD(ins, rd, false);

	return 1;
}

u8 Arm::executeThumbMisc(ThumbInstruction& ins)
{
	bool add_sub_sp = (((ins.encoding >> 8) & 0xF) == 0b0000);
	bool push_pop_reg_list = (((ins.encoding >> 9) & 0x3) == 0b10);

	if (add_sub_sp) {
		u8 opc = ((ins.encoding) >> 7) & 0x1;
		//Misc Add/Sub SP
		if (opc == 0x0) {
			thumbOpADD(ins, ins.imm7());
		}else{
			thumbOpSUB(ins, ins.imm7());
		}
	}
	else if (push_pop_reg_list) {

	}

	return 1;
}

u8 Arm::handleUndefinedThumbIns(ThumbInstruction& ins)
{
	u8 lutIndex = ins.instruction();
	printf("LUT Index: %d ", lutIndex);
	printf("THUMB undefined or unimplemented instruction 0x%04X at PC: 0x%08X\n", ins.encoding, R15 - 8);

	return 1;
}

u8 Arm::opMOV(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);
	
	if (rd.id == 0xF) {
		flushPipeline();
	}
	reg_rd = shifter_op;
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}

u8 Arm::opADD(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	//Shifter carry out is not needed here
	u8 shifter_carry_out = 0;
	bool carry = false;
	bool overflow = false;

	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn + shifter_op;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	carry = carryFrom(reg_rn, shifter_op);
	overflow = overflowFromAdd(reg_rn, shifter_op);
	
	if (flags) {
		setCC(reg_rd, !carry, overflow);
	}

	return 1;
}

u8 Arm::opAND(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn & shifter_op;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}


u8 Arm::opEOR(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn ^ shifter_op;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}

u8 Arm::opSUB(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	bool borrow = false;
	bool overflow = false;

	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn - shifter_op;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	borrow = borrowFrom(reg_rn, shifter_op);
	overflow = overflowFromSub(reg_rn, shifter_op);
	
	if (flags) {
		setCC(reg_rd, borrow, overflow);
	}

	return 1;
}

u8 Arm::opRSB(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	bool borrow = false;
	bool overflow = false;

 	u32 shifter_op = (immediate == true) ? 
		addrMode1.imm(ins, shifter_carry_out) :  addrMode1.shift(ins, shifter_carry_out);
	
	u32 result = shifter_op - reg_rn;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	borrow = borrowFrom(shifter_op, reg_rn);
	overflow = overflowFromSub(shifter_op, reg_rn);

	if (flags) {
		setCC(reg_rd, !borrow, overflow);
	}

	return 1;
}

u8 Arm::opADC(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	bool carry = false;
	bool overflow = false;

	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn + (shifter_op + getFlag(C));
	reg_rd = result;
	writeRegister(rd, reg_rd);

	carry = carryFrom(reg_rn, shifter_op + getFlag(C));
	overflow = overflowFromAdd(reg_rn, shifter_op + getFlag(C));

	if (flags) {
		setCC(reg_rd, !carry, overflow);
	}

	return 1;
}

u8 Arm::opSBC(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	bool borrow = false;
	bool overflow = false;

	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn - (shifter_op - !getFlag(C));
	reg_rd = result;
	writeRegister(rd, reg_rd);

	borrow = borrowFrom(reg_rn, shifter_op - !getFlag(C));
	overflow = overflowFromSub(reg_rn, shifter_op - !getFlag(C));

	if (flags) {
		setCC(reg_rd, !borrow, overflow);
	}

	return 1;
}

u8 Arm::opRSC(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	bool borrow = false;
	bool overflow = false;

	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = shifter_op - (reg_rn - !getFlag(C));
	reg_rd = result;
	writeRegister(rd, reg_rd);

	borrow = borrowFrom(shifter_op, (reg_rn - !getFlag(C)));
	overflow = overflowFromSub(shifter_op, (reg_rn - !getFlag(C)));

	if (flags) {
		setCC(reg_rd, !borrow, overflow);
	}

	return 1;
}

u8 Arm::opTST(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn & shifter_op;

	(result >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(result == 0) ? setFlag(Z) : clearFlag(Z);
	(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	

	return 1;
}

u8 Arm::opTEQ(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn ^ shifter_op;

	(result >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(result == 0) ? setFlag(Z) : clearFlag(Z);
	(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);


	return 1;
}

u8 Arm::opCMP(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn - shifter_op;

	bool borrow = borrowFrom(reg_rn, shifter_op);
	bool overflow = overflowFromSub(reg_rn, shifter_op);

	setCC(result, borrow, overflow);

	return 1;
}

u8 Arm::opCMN(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn + shifter_op;

	bool carry = carryFrom(reg_rn, shifter_op);
	bool overflow = overflowFromAdd(reg_rn, shifter_op);

	setCC(result, !carry, overflow);

	return 1;
}

u8 Arm::opORR(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn | shifter_op;
	reg_rd = result;
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}

u8 Arm::opBIC(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	u32 result = reg_rn & ~(shifter_op);
	reg_rd = result;
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}

u8 Arm::opMVN(ArmInstruction& ins, RegisterID rd, RegisterID rn,
	bool flags, bool immediate)
{
	u32 reg_rd = getRegister(rd);

	u8 shifter_carry_out = 0;
	u32 shifter_op = (immediate == true) ?
		addrMode1.imm(ins, shifter_carry_out) : addrMode1.shift(ins, shifter_carry_out);

	reg_rd = ~(shifter_op);
	writeRegister(rd, reg_rd);

	if (flags) {
		setCC(reg_rd, false, false, true, shifter_carry_out);
	}

	return 1;
}

u8 Arm::opB(ArmInstruction& ins)
{
	s32 offset = ins.offset();
	offset = signExtend32(offset, 24);
	//Aligns r15 for arm instruction reading
	offset <<= 2;

	R15 += offset;
	flushPipeline();

	return 1;
}

u8 Arm::opBL(ArmInstruction& ins)
{
	LR = R15 - 4;
	
	s32 offset = ins.offset();
	offset = signExtend32(offset, 24);
	offset <<= 2;

	R15 += offset;
	flushPipeline();

	return 1;
}

u8 Arm::opBX(ArmInstruction& ins)
{
	RegisterID rm = ins.rm();
	u32 reg_rm = getRegister(rm);
	
	(reg_rm & 0x1) == 1 ? setFlag(T) : clearFlag(T);
	R15 = reg_rm & 0xFFFFFFFE;

	if (getFlag(T) == 0x0) {
		flushPipeline();
	}
	else {
		flushThumbPipeline();
	}
	R15 -= 2;

	return 1;
}

u8 Arm::opSWI(ArmInstruction& ins)
{
	LR_svc = R15 - 4;
	SPSR_svc = CPSR;

	enterSupervisorMode();
	clearFlag(T);
	setFlag(I);

	R15 = 0x8;
	flushPipeline();

	return 1;
}


u8 Arm::opSTRH(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u32 reg_rd = getRegister(rd);
	mbus->writeU16(address, reg_rd & 0xFFFF);

	return 1;
}


u8 Arm::opLDRH(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u16 value = mbus->readU16(address);
	writeRegister(rd, value);

	return 1;
}

u8 Arm::opLDRSB(ArmInstruction& ins, RegisterID rd, u32 address)
{
	s8 value = (s8)mbus->readU8(address);
	value = signExtend32(value, 8);
	writeRegister(rd, value);

	return 1;
}

u8 Arm::opLDRSH(ArmInstruction& ins, RegisterID rd, u32 address)
{
	s16 value = (s16)mbus->readU16(address);
	value = signExtend32(value, 16);
	writeRegister(rd, value);

	return 1;
}

u8 Arm::opLDRB(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u8 value = mbus->readU8(address);
	writeRegister(rd, value);

	return 1;
}

u8 Arm::opLDR(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u32 value = mbus->readU32(address);
	writeRegister(rd, value);

	return 1;
}

u8 Arm::opSTRB(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u32 reg_rd = getRegister(rd);
	mbus->writeU8(address, reg_rd & 0xFF);

	return 1;
}

u8 Arm::opSTR(ArmInstruction& ins, RegisterID rd, u32 address)
{
	u32 reg_rd = getRegister(rd);
	mbus->writeU32(address, reg_rd);

	return 1;
}

u8 Arm::opMSR(ArmInstruction& ins, u32 value)
{
	u8 R = ins.R();
	u8 fm = ins.fieldMask();
	bool cpsr_write = (R == 0x0);

	if (cpsr_write) {
		if ((fm & 0x1) == 0x1) {
			u32 operand = value & 0xFF;
			for (u32 i = 0; i <= 7; i++)
				CPSR = resetBit(CPSR, i);

			CPSR |= operand;
		}
		if (((fm >> 3) & 0x1) == 0x1) {
			u32 operand = (value >> V_BIT) & 0xF;
			for (u32 i = 28; i <= 31; i++)
				CPSR = resetBit(CPSR, i);

			CPSR |= (operand << V_BIT);
		}
	}
	else { //spsr write
		if ((fm & 0x1) == 0x1) {
			u32 operand = value & 0xFF;
			for (u32 i = 0; i <= 7; i++)
				SPSR = resetBit(SPSR, i);

			SPSR |= operand;
		}
		if (((fm >> 3) & 0x1) == 0x1) {
			u32 operand = (value >> V_BIT) & 0xF;
			for (u32 i = 28; i <= 31; i++)
				SPSR = resetBit(SPSR, i);

			SPSR |= (operand << V_BIT);
		}
	}

	return 1;
}

//Thumb instructions

u8 Arm::thumbOpBCond(ThumbInstruction& ins)
{
	u8 cond = getConditionCode(ins.cond());
	if (cond) {
		s32 signed_imm8_offset = ins.signedImm8();
		signed_imm8_offset = signExtend32(signed_imm8_offset, 8);
		//Aligns r15 for thumb instruction reading
		signed_imm8_offset <<= 1;

		R15 += signed_imm8_offset;

		flushThumbPipeline();
	}

	return 1;
}

u8 Arm::thumbOpB(ThumbInstruction& ins)
{
	s32 signed_imm11_offset = ins.signedImm11();
	signed_imm11_offset = signExtend32(signed_imm11_offset, 11);
	signed_imm11_offset <<= 1;

	R15 += signed_imm11_offset;

	flushThumbPipeline();

	return 1;
}

u8 Arm::thumbOpBL(ThumbInstruction& ins)
{
	u8 H = ins.H();
	switch (H) {
		case 0b10:
		{
			s32 offset11 = ins.offset11();
			offset11 = signExtend32(offset11, 11);
			offset11 <<= 12;
	
			LR = R15 + offset11;
		}
		break;

		case 0b11:
		{
			s32 offset11 = ins.offset11();
			offset11 = signExtend32(offset11, 11);
			offset11 <<= 1;

			u32 prevLR = LR;
			LR = prevLR | 0x1;
			R15 = prevLR + offset11;

			flushThumbPipeline();
		}
		break;
	}
	
	return 1;
}

u8 Arm::thumbOpLDRPool(ThumbInstruction& ins)
{
	RegisterID rd = ins.rdUpper();
	u8 imm8 = ins.imm8();

	//lower 2 bits of PC are disregarded/shifted left to word align
	u32 address = (R15 & 0xFFFFFFFC) + (imm8 * 4);
	u32 value = mbus->readU32(address);

	writeRegister(rd, value);

	return 1;
}

u8 Arm::thumbOpLDR(ThumbInstruction& ins)
{
	RegisterID rd = ins.rdLower();
	RegisterID rn = ins.rnMiddle();
	RegisterID rm = ins.rmUpper();

	u32 reg_rn = getRegister(rn);
	u32 reg_rm = getRegister(rm);
	u32 address = reg_rn + reg_rm;

	u32 value;
	if ((address & 0x3) == 0b00) { //If the address is word aligned, read the value from memory
		value = mbus->readU32(address);
		writeRegister(rd, value);
	}


	return 1;
}

u8 Arm::thumbOpASR(ThumbInstruction& ins, u8 immediate5)
{
	RegisterID rd = ins.rdLower();
	RegisterID rm = ins.rmLower();

	u32 reg_rd = getRegister(rd);
	u32 reg_rm = getRegister(rm);

	//shift by 32
	if (immediate5 == 0) {
		bool rm_sign_bit = (reg_rm >> 31) & 0x1;
		(rm_sign_bit == 1) ? setFlag(C) : clearFlag(C);

		if (rm_sign_bit == 0) {
			reg_rd = 0x0;
		}
		else {
			reg_rd = 0xFFFFFFFF;
		}
	}
	//imm5 > 0
	else {
		u8 shifter_carry_out = 0;
		reg_rd = asr(reg_rm, immediate5, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpASR(ThumbInstruction& ins, RegisterID rs, RegisterID rd)
{
	u32 reg_rs = getRegister(rs);
	u32 reg_rd = getRegister(rd);

	u8 shift_amount = reg_rs & 0xFF;
	//shift by 32
	if (shift_amount == 0x0) {
		//C flag unaffected
		//rd unaffected
	}
	else if (shift_amount < 32) {
		u8 shifter_carry_out = 0;
		reg_rd = asr(reg_rd, shift_amount, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	//shift_amount >= 32
	else {
		bool rd_sign_bit = (reg_rd >> 31) & 0x1;
		(rd_sign_bit == 1) ? setFlag(C) : clearFlag(C);

		if (rd_sign_bit == 0) {
			reg_rd = 0x0;
		}
		else {
			reg_rd = 0xFFFFFFFF;
		}
	}
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpLSL(ThumbInstruction& ins, u8 immediate5)
{
	RegisterID rd = ins.rdLower();
	RegisterID rm = ins.rmLower();

	u32 reg_rd = getRegister(rd);
	u32 reg_rm = getRegister(rm);

	//if zero, Simply load the register
	if (immediate5 == 0) {
		//C flag unaffected
		reg_rd = reg_rm;
	}
	//imm5 > 0
	else {
		u8 shifter_carry_out = 0;
		reg_rd = lsl(reg_rm, immediate5, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	writeRegister(rd, reg_rd);
	
	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpLSL(ThumbInstruction& ins, RegisterID rs, RegisterID rd)
{
	u32 reg_rs = getRegister(rs);
	u32 reg_rd = getRegister(rd);

	u8 shift_amount = reg_rs & 0xFF;
	if (shift_amount == 0) {
		//C flag unaffected
		//rd unaffected
	}
	else if (shift_amount < 32) {
		u8 shifter_carry_out = 0;
		reg_rd = lsl(reg_rd, shift_amount, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	else if (shift_amount == 32) {
		(reg_rd & 0x1) ? setFlag(C) : clearFlag(C);
		reg_rd = 0x0;
	}
	//shift_amount > 32
	else {
		clearFlag(C);
		reg_rd = 0x0;
	}
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpLSR(ThumbInstruction& ins, u8 immediate5)
{
	RegisterID rm = ins.rmLower();
	RegisterID rd = ins.rdLower();

	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	//shift by 32
	if (immediate5 == 0) {
		(reg_rd >> 31) & 0x1 ? setFlag(C) : clearFlag(C);
		reg_rd = 0x0;
	}
	//imm5 > 0
	else {
		u8 shifter_carry_out = 0;
		reg_rd = lsr(reg_rm, immediate5, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpLSR(ThumbInstruction& ins, RegisterID rs, RegisterID rd)
{
	u32 reg_rs = getRegister(rs);
	u32 reg_rd = getRegister(rd);

	u8 shift_amount = reg_rs & 0xFF;

	if (shift_amount == 0) {
		//C flag unaffected
		//rd unaffected
	}
	else if (shift_amount < 32) {
		u8 shifter_carry_out = 0;
		reg_rd = lsr(reg_rd, shift_amount, shifter_carry_out);

		(shifter_carry_out == 1) ? setFlag(C) : clearFlag(C);
	}
	else if (shift_amount == 32) {
		(reg_rd >> 31) & 0x1 ? setFlag(C) : clearFlag(C);
		reg_rd = 0x0;
	}
	//shift_amount > 32
	else {
		clearFlag(C);
		reg_rd = 0x0;
	}
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpMOV(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{	
	u8 h1 = ins.h1();
	u8 h2 = ins.h2();

	RegisterID actual_reg_rm;
	actual_reg_rm.id = (h2 << 3) | rm.id;

	RegisterID actual_reg_rd;
	actual_reg_rd.id = (h1 << 3) | rd.id;

	u32 reg_rm = getRegister(actual_reg_rm);

	writeRegister(actual_reg_rd, reg_rm);

	return 1;
}

u8 Arm::thumbOpMOV(ThumbInstruction& ins, u8 immediate)
{
	RegisterID rd = ins.rdUpper();
	u32 reg_rd = getRegister(rd);

	reg_rd = immediate;
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
	
	return 1;
}

u8 Arm::thumbOpMUL(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rm * reg_rd;
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpNEG(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = 0 - reg_rm;
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	bool borrow = borrowFrom(0, reg_rm);
	bool overflow = overflowFromSub(0, reg_rm);

	setCC(reg_rd, borrow, overflow);

	return 1;
}

u8 Arm::thumbOpMVN(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = ~(reg_rm);
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpADC(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	u32 result = reg_rd + reg_rm + getFlag(C);
	writeRegister(rd, result);

	bool carry = carryFrom(reg_rd, reg_rm + getFlag(C));
	bool overflow = overflowFromAdd(reg_rd, reg_rm + getFlag(C));

	setCC(result, !carry, overflow);

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, RegisterID rm, RegisterID rn, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rn + reg_rm;
	writeRegister(rd, reg_rd);

	bool carry = carryFrom(reg_rn, reg_rm);
	bool overflow = overflowFromAdd(reg_rn, reg_rm);

	setCC(reg_rd, !carry, overflow);

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, RegisterID rn, RegisterID rd, u8 immediate)
{
	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	//it's a mov encoded as add rd, rn, #0
	if (immediate == 0) {
		reg_rd = reg_rn;
		writeRegister(rd, reg_rd);

		(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
		(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
		clearFlag(C);
		clearFlag(V);
	}
	else {
		reg_rd = reg_rn + immediate;
		writeRegister(rd, reg_rd);

		bool carry = carryFrom(reg_rn, immediate);
		bool overflow = overflowFromAdd(reg_rn, immediate);

		setCC(reg_rd, !carry, overflow);
	}

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, RegisterID rd, u8 immediate)
{
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rd + immediate;
	writeRegister(rd, reg_rd);

	bool carry = carryFrom(reg_rd, immediate);
	bool overflow = overflowFromAdd(reg_rd, immediate);

	setCC(reg_rd, !carry, overflow);

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u8 h1 = ins.h1();
	u8 h2 = ins.h2();

	RegisterID actual_reg_rm_id;
	actual_reg_rm_id.id = (h2 << 3) | rm.id;

	RegisterID actual_reg_rd_id;
	actual_reg_rd_id.id = (h1 << 3) | rd.id;

	u32 reg_rm = getRegister(actual_reg_rm_id);
	u32 reg_rd = getRegister(actual_reg_rd_id);

	reg_rd = reg_rd + reg_rm;
	writeRegister(actual_reg_rd_id, reg_rd);

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, RegisterID rd, bool pc)
{
	u32 reg_rd = getRegister(rd);
	u8 imm8 = ins.imm8();
	//Add pc
	if (pc) {
		reg_rd = (R15 & 0xFFFFFFFC) + (imm8 * 4);
	}
	//add sp
	else {
		u32 sp = getRegister(RegisterID{ R13_ID });
		reg_rd = sp + (imm8 * 4);
	}

	writeRegister(rd, reg_rd);

	return 1;
}

u8 Arm::thumbOpADD(ThumbInstruction& ins, u8 immediate7)
{
	u32 sp = getRegister(RegisterID{ R13_ID });

	sp = sp + (immediate7 * 4);
	writeRegister(RegisterID{ R13_ID }, sp);

	return 1;
}

u8 Arm::thumbOpAND(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rd & reg_rm;
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpCMN(ThumbInstruction& ins, RegisterID rm, RegisterID rn)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rn = getRegister(rn);

	u32 result = reg_rn + reg_rm;

	(result >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(result == 0) ? setFlag(Z) : clearFlag(Z);

	bool carry = carryFrom(reg_rn, reg_rm);
	bool overflow = overflowFromAdd(reg_rn, reg_rm);

	setCC(result, !carry, overflow);

	return 1;
}

u8 Arm::thumbOpCMP(ThumbInstruction& ins, RegisterID rm, RegisterID rn, bool hiRegisters)
{
	if (hiRegisters) {
		u8 h1 = ins.h1();
		u8 h2 = ins.h2();

		RegisterID actual_reg_rn_id;
		actual_reg_rn_id.id = (h1 << 3) | rn.id;

		RegisterID actual_reg_rm_id;
		actual_reg_rm_id.id = (h2 << 3) | rm.id;

		u32 reg_rn = getRegister(actual_reg_rn_id);
		u32 reg_rm = getRegister(actual_reg_rm_id);
		
		u32 result = reg_rn - reg_rm;

		bool borrow = borrowFrom(reg_rn, reg_rm);
		bool overflow = overflowFromSub(reg_rn, reg_rm);

		setCC(result, borrow, overflow);
	}
	else {

		u32 reg_rm = getRegister(rm);
		u32 reg_rn = getRegister(rn);

		u32 result = reg_rn - reg_rm;

		bool borrow = borrowFrom(reg_rn, reg_rm);
		bool overflow = overflowFromSub(reg_rn, reg_rm);

		setCC(result, borrow, overflow);
	}

	return 1;
}

u8 Arm::thumbOpCMP(ThumbInstruction& ins, u8 immediate)
{
	RegisterID rn = ins.rnUpper();
	u32 reg_rn = getRegister(rn);

	u32 result = reg_rn - immediate;

	bool borrow = borrowFrom(reg_rn, immediate);
	bool overflow = overflowFromSub(reg_rn, immediate);

	setCC(result, borrow, overflow);

	return 1;
}

u8 Arm::thumbOpEOR(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rd ^ reg_rm;
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpSUB(ThumbInstruction& ins, RegisterID rm, RegisterID rn, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rn - reg_rm;
	writeRegister(rd, reg_rd);

	bool borrow = borrowFrom(reg_rn, reg_rm);
	bool overflow = overflowFromSub(reg_rn, reg_rm);

	setCC(reg_rd, borrow, overflow);

	return 1;
}

u8 Arm::thumbOpSUB(ThumbInstruction& ins, RegisterID rn, RegisterID rd, u8 immediate)
{
	u32 reg_rn = getRegister(rn);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rn - immediate;
	writeRegister(rd, reg_rd);

	bool borrow = borrowFrom(reg_rn, immediate);
	bool overflow = overflowFromSub(reg_rn, immediate);

	setCC(reg_rd, borrow, overflow);

	return 1;
}

u8 Arm::thumbOpSUB(ThumbInstruction& ins, RegisterID rd, u8 immediate)
{
	u32 reg_rd = getRegister(rd);

	u32 src_reg = reg_rd;
	reg_rd = src_reg - immediate;
	writeRegister(rd, reg_rd);

	//bool borrow = borrowFrom(reg_rd, immediate);
	bool overflow = overflowFromSub(reg_rd, immediate);

	//setCC(reg_rd, borrow, overflow);
	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
	((u64)immediate > src_reg) ? clearFlag(C) : setFlag(C);
	(overflow == true) ? setFlag(V) : clearFlag(V);

	return 1;
}

u8 Arm::thumbOpSUB(ThumbInstruction& ins, u8 immediate7)
{
	u32 sp = getRegister(RegisterID{ R13_ID });

	sp = sp - (immediate7 * 4);
	writeRegister(RegisterID{ R13_ID }, sp);

	return 1;
}

u8 Arm::thumbOpBIC(ThumbInstruction& ins, RegisterID rm, RegisterID rd)
{
	u32 reg_rm = getRegister(rm);
	u32 reg_rd = getRegister(rd);

	reg_rd = reg_rd & ~(reg_rm);
	writeRegister(rd, reg_rd);

	(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
	(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);

	return 1;
}

u8 Arm::thumbOpSTMIA(ThumbInstruction& ins)
{
	RegisterID rn = ins.rnUpper();
	u32 reg_rn = getRegister(rn);

	u8 reg_list = ins.registerList();
	u32 address = reg_rn;
	u32 end_address = reg_rn + (numSetBitsU8(reg_list) * 4) - 4;

	for (s32 i = 0; i <= 7; i++) {
		bool included = testBit(reg_list, i);
		if (included) {
			RegisterID id; id.id = i;

			u32 reg = getRegister(id);
			mbus->writeU32(address, reg);
			address += 4;
		}

		if (end_address == address - 4) break;
	}
	reg_rn += (numSetBitsU8(reg_list) * 4);
	writeRegister(rn, reg_rn);

	return 1;
}

u8 Arm::thumbOpLDMIA(ThumbInstruction& ins)
{
	RegisterID rn = ins.rnUpper();
	u32 reg_rn = getRegister(rn);

	u8 reg_list = ins.registerList();
	u32 address = reg_rn;
	u32 end_address = reg_rn + (numSetBitsU8(reg_list) * 4) - 4;

	for (s32 i = 0; i <= 7; i++) {
		bool included = testBit(reg_list, i);
		if (included) {
			RegisterID id; id.id = i;
			
			u32 value = mbus->readU32(address);
			writeRegister(id, value);
			address += 4;
		}

		if (end_address == address - 4) break;
	}
	reg_rn += (numSetBitsU8(reg_list) * 4);
	writeRegister(rn, reg_rn);

	return 1;
}

u8 Arm::thumbOpBX(ThumbInstruction& ins)
{
	RegisterID rm = ins.rmLower();
	u8 h2 = ins.h2();

	RegisterID actual_reg_id;
	actual_reg_id.id = ((h2 << 3) | rm.id) & 0xF;

	u32 reg = getRegister(actual_reg_id);

	(reg & 0x1) == 1 ? setFlag(T) : clearFlag(T);
	R15 = reg & 0xFFFFFFFE;

	if (getFlag(T) == 0x0) {
		flushPipeline();
	}
	else {
		flushThumbPipeline();
	}

	return 1;
}

u8 Arm::thumbOpSWI(ThumbInstruction& ins)
{
	LR_svc = R15 - 2; //store address of next instruction after this one
	SPSR_svc = CPSR;

	enterSupervisorMode();
	clearFlag(T); //Execute in arm state
	setFlag(I); //disable normal interrupts

	R15 = 0x8; //jump to bios
	flushPipeline();

	return 1;
}



