#include "Arm.h"
#include "../Memory/MemoryBus.h"

Arm::Arm(MemoryBus *mbus)
	:addrMode1(*this)
{
	this->mbus = mbus;
	reset();
}

u8 Arm::clock()
{
	state = getState();
	if (state == State::ARM) {
		u32 encoding = armpipeline[0];
		armpipeline[0] = armpipeline[1];
		armpipeline[1] = fetchU32();

		ArmInstruction ins;
		ins.encoding = encoding;
		executeArmIns(ins);
	}
	else if (state == State::THUMB) {
		u32 encoding = thumbpipeline[0];
		thumbpipeline[0] = thumbpipeline[1];
		thumbpipeline[1] = fetchU16();

		ThumbInstruction ins;
		ins.encoding = encoding;
		executeThumbIns(ins);
	}
	return cycles;
}

void Arm::reset()
{
	cycles = 0;
	for (s32 i = 0; i < 0xD; i++) {
		registers[i].value = 0x00000000;
	}

	LR = 0x00000000;
	R15 = 0x80000000;
	SP = 0x00000000;
	CPSR = 0x6000001F;
	SPSR = 0x03007F00;
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
		s64 result = op1 + op2;
		if (result & signBit) return true;
	}
	if (negative) {
		s64 result = op1 + op2;
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

u32 Arm::getPC()
{
	u32 pc = ((R15 >> 2) & 0xFFFFFFFF);
	return pc;
}

PSR Arm::getPSR()
{
	return PSR{
		CPSR,
		SPSR
	};
}

u32 Arm::getRegister(RegisterID reg)
{
	if (reg.id >= 0xD) {
		if (reg.id == 0xD) return SP;
		if (reg.id == 0xE) return LR;
		if (reg.id == 0xF) return R15;
	}
	return registers[reg.id].value;
}

void Arm::writeRegister(RegisterID reg, u32 value)
{
	if (reg.id >= 0xD) {
		if (reg.id == 0xD) SP = value;
		if (reg.id == 0xE) LR = value;
		if (reg.id == 0xF) R15 = value;

		return;
	}
	registers[reg.id].value = value;
}

void Arm::writePC(u32 pc)
{
	R15 |= ((pc << 2) & 0xFFFFFFFF);
}

State Arm::getState()
{
	u8 st = ((CPSR & T) >> T_BIT);
	return State(st);
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
	return u16();
}

u16 Arm::fetchU16()
{
	return u16();
}

u32 Arm::readU32()
{
	u32 PC = getPC();

	u32 word = (mbus->readU8(PC)) |
		(mbus->readU8(PC + 1) << 8) |
		(mbus->readU8(PC + 2) << 16) |
		(mbus->readU8(PC + 3) << 24);

	return word;
}

u32 Arm::fetchU32()
{
	u32 PC = getPC();

	u32 word = (mbus->readU8(PC++)) |
		(mbus->readU8(PC++) << 8) |
		(mbus->readU8(PC++) << 16) |
		(mbus->readU8(PC++) << 24);

	writePC(PC);

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

	u32 result = rotatedIn | (rotatedOut << (31 - shift));
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
	switch (ins.opcode()) {
		case 0b0000: opAND(ins, getConditionCode(ins.cond()), ins.rd(), ins.rn()); break;
		case 0b0001: opEOR(ins, getConditionCode(ins.cond()), ins.rd(), ins.rn()); break;
		case 0b0010: opSUB(ins, getConditionCode(ins.cond()), ins.rd(), ins.rn()); break;
		case 0b1101: opMOV(ins, getConditionCode(ins.cond()), ins.rd(), ins.rn()); break;
		case 0b0100: opADD(ins, getConditionCode(ins.cond()), ins.rd(), ins.rn()); break;
	}
}

void Arm::executeThumbIns(ThumbInstruction& ins)
{
}

u8 Arm::opMOV(ArmInstruction& ins, u8 condition, RegisterID rd, RegisterID rn)
{
	u32 reg_rd = getRegister(rd);

	bool set = (ins.s() == 0x0) ? false : true;
	bool immediate = (ins.i() == 0x0) ? false : true;
	
	if (condition) {
		u8 shiftedBit = 0;
		if (immediate) {
			//op2 is an immediate value
			u32 result = addrMode1.imm(ins, shiftedBit);
			reg_rd = result;	
		}
		else {
			//op2 is a register
			u32 result = addrMode1.shift(ins, shiftedBit);
			reg_rd = result;
		}
		writeRegister(rd, reg_rd);

		if (set) {
			if (reg_rd != R15) {
				(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
				(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
				(shiftedBit == 1) ? setFlag(C) : clearFlag(C);
			}
			else {
				//spsr of curr mode is copied to cpsr
				CPSR = SPSR;
			}

		}
	}

	return 1;
}

u8 Arm::opADD(ArmInstruction& ins, u8 condition, RegisterID rd, RegisterID rn)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	bool set = (ins.s() == 0x0) ? false : true;
	bool immediate = (ins.i() == 0x0) ? false : true;

	if (condition) {
		//Shifter carry out(shifted bit) is not needed here
		u8 shiftedBit = 0;
		bool carry = false;
		bool overflow = false;

		if (immediate) {
			u32 shifter_op = addrMode1.imm(ins, shiftedBit);
			u32 result = reg_rn + shifter_op;
			reg_rd = result;

			carry = carryFrom(reg_rn, shifter_op);
			overflow = overflowFromAdd(reg_rn, shifter_op);
		}
		else {
			u32 shifter_op = addrMode1.shift(ins, shiftedBit);
			u32 result = reg_rn + shifter_op;
			reg_rd = result;

			carry = carryFrom(reg_rn, shifter_op);
			overflow = overflowFromAdd(reg_rn, shifter_op);
		}
		writeRegister(rd, reg_rd);

		if (set) {
			if (reg_rd != R15) {
				(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
				(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
				(carry == true) ? setFlag(C) : clearFlag(C);
				(overflow == true) ? setFlag(V) : clearFlag(V);
			}
			else {
				CPSR = SPSR;
			}
		}
	}

	return 1;
}

u8 Arm::opAND(ArmInstruction& ins, u8 condition, RegisterID rd, RegisterID rn)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	bool set = (ins.s() == 0x0) ? false : true;
	bool immediate = (ins.i() == 0x0) ? false : true;

	if (condition) {
		u8 shiftedBit = 0;
		if (immediate) {
			u32 shifter_op = addrMode1.imm(ins, shiftedBit);
			u32 result = reg_rn & shifter_op;
			reg_rd = result;
		}
		else {
			u32 shifter_op = addrMode1.shift(ins, shiftedBit);
			u32 result = reg_rn & shifter_op;
			reg_rd = result;
		}
		writeRegister(rd, reg_rd);

		if (set) {
			if (reg_rd != R15) {
				(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
				(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
				(shiftedBit == 1) ? setFlag(C) : clearFlag(C);
			}
			else {
				CPSR = SPSR;
			}
		}
	}

	return 1;
}


u8 Arm::opEOR(ArmInstruction& ins, u8 condition, RegisterID rd, RegisterID rn)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	bool set = (ins.s() == 0x0) ? false : true;
	bool immediate = (ins.i() == 0x0) ? false : true;

	if (condition) {
		u8 shiftedBit = 0;
		if (immediate) {
			u32 shifter_op = addrMode1.imm(ins, shiftedBit);
			u32 result = reg_rn ^ shifter_op;
			reg_rd = result;
		}
		else {
			u32 shifter_op = addrMode1.shift(ins, shiftedBit);
			u32 result = reg_rn ^ shifter_op;
			reg_rd = result;
		}
		writeRegister(rd, reg_rd);

		if (set) {
			if (reg_rd != R15) {
				(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
				(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
				(shiftedBit == 1) ? setFlag(C) : clearFlag(C);
			}
			else {
				CPSR = SPSR;
			}
		}
	}

	return 1;
}

u8 Arm::opSUB(ArmInstruction& ins, u8 condition, RegisterID rd, RegisterID rn)
{
	u32 reg_rd = getRegister(rd);
	u32 reg_rn = getRegister(rn);

	bool set = (ins.s() == 0x0) ? false : true;
	bool immediate = (ins.i() == 0x0) ? false : true;

	if (condition) {
		u8 shiftedBit = 0;
		bool borrow = false;
		bool overflow = false;

		if (immediate) {
			u32 shifter_op = addrMode1.imm(ins, shiftedBit);
			u32 result = reg_rn - shifter_op;
			reg_rd = result;

			borrow = borrowFrom(reg_rn, shifter_op);
			overflow = overflowFromSub(reg_rn, shifter_op);
		}
		else {
			u32 shifter_op = addrMode1.shift(ins, shiftedBit);
			u32 result = reg_rn - shifter_op;
			reg_rd = result;

			borrow = borrowFrom(reg_rn, shifter_op);
			overflow = overflowFromSub(reg_rn, shifter_op);
		}
		writeRegister(rd, reg_rd);

		if (set) {
			if(reg_rd != R15) {
				(reg_rd >> 31) & 0x1 ? setFlag(N) : clearFlag(N);
				(reg_rd == 0) ? setFlag(Z) : clearFlag(Z);
				(borrow == false) ? setFlag(C) : clearFlag(C);
				(overflow == true) ? setFlag(V) : clearFlag(V);
			}
			else {
				CPSR = SPSR;
			}
		}
	}

	return 1;
}

