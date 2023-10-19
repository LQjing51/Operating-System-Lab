<h3>实验项目一</h3>
<p><strong>一、实验简介</strong></p>
<ul>
<li>task1：在引导程序bootblock.s中实现打印字符串的功能，熟悉上板流程。</li></ul>

<ul>
<li>task2: 补全Boot Loader的加载内核部分代码，调用SBI将位于SD卡第二个扇区的内核代码移动至内存bootloader后面。在head.S中清空BSS栈，设置栈指针。在kernel.c中，调用SBI，打印字符串“Hello OS”，然后继续接收用户输入，并实时打印出来。</li>

</ul>
<ul>
<li>task2(A core):将内核拷贝到内存的0x50200000处，并从此处开始执行代码。</li>
</ul>

<ul>
<li>task3:编写createimage.c代码，由多个可执行文件生成二进制文件image。</li>

</ul>
<ul>
<li>task3(A core):修改createimage.c程序，使其支持各种各样的kernel大小。</li>

</ul>
<ul>
<li>task4(C core):准备两个内核，启动后实现双系统选择引导的功能。</li>
</ul>

<p><strong>二、完成情况与使用说明</strong></p>
<p>	最终提交的代码完成了以上所有等级（C core）的实验内容。其中kernel1是用的老师提供的2048游戏的大核，kernel0是打印“Hello OS”等信息的小核。</p>
<p>	当板卡上电执行到bootblock时，会询问用户输入。此时若输入‘0’字符（注意，这里不会打印用户的输入），则拷贝kernel0到内核入口，输入其它字符，则拷贝kernel1到内核入口。</p>
<p>	进入kernel0后，在接受用户输入的部分，按Ctrl+D可以结束用户输入。</p>