#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU]; // Ϊÿ�� CPU ���� 4096 �ֽڵ�ջ

// entry.S jumps here in machine mode on stack0.
void start() {
    // ���� M Previous Privilege mode Ϊ Supervisor, ���� mret��
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK; // ��� MPP λ
    x |= MSTATUS_MPP_S;     // ����Ϊ Supervisor ģʽ
    w_mstatus(x);          // д�� MSTATUS �Ĵ���

    // ���� M Exception Program Counter Ϊ main��׼�� mret��
    w_mepc((uint64)main);

    // ���ڽ��÷�ҳ��
    w_satp(0);

    // �������жϺ��쳣ί�и� Supervisor ģʽ��
    w_medeleg(0xffff);
    w_mideleg(0xffff);
    w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE); // ʹ�� Supervisor ģʽ�µ��ж�

    // ���������ڴ汣�������� Supervisor ģʽ�������������ڴ档
    w_pmpaddr0(0x3fffffffffffffull);
    w_pmpcfg0(0xf);

    // ����ʱ���жϡ�
    timerinit();

    // ��ÿ�� CPU �� hartid �洢���� tp �Ĵ����У��Թ� cpuid() ʹ�á�
    int id = r_mhartid();
    w_tp(id);

    // �л��� Supervisor ģʽ����ת�� main()�� 
    asm volatile("mret");
}

// ����ÿ�� hart ���ɶ�ʱ���жϡ�
void timerinit() {
    // ���� Supervisor ģʽ�µĶ�ʱ���жϡ�
    w_mie(r_mie() | MIE_STIE);
    
    // ���� sstc ��չ���� stimecmp�����ü��ģʽ���Ը��õع���ʱ�����������ö�ʱ���жϵȣ��Ӷ��������ϵͳ��ʱ���¼����д���
    w_menvcfg(r_menvcfg() | (1L << 63)); 
    
    // ���� Supervisor ʹ�� stimecmp �� time��ͨ������ Supervisor ʹ�� stimecmp �� time������ϵͳ�ܹ���Ч�����ü�ʱ������ʱ�������¼����ȡ�����ζ���ܸ������ʵ�ֶ�ʱ���жϡ����ȵȹ��ܣ��Ը��õ�֧�ֶ����񻷾���
    w_mcounteren(r_mcounteren() | 2);
    
    // �����һ�ζ�ʱ���жϡ�
    w_stimecmp(r_time() + 1000000);
}