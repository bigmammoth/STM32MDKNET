t <html><head><title>��������</title>
t <script language=JavaScript>
t function changeConfirm(f){
t  if(!confirm('ȷ��Ҫ�޸������������?')) return;
t  f.submit();
t }
t </script></head>
i pg_header.inc
t <h2 align=center><br>��������</h2>
t <p><font size="3">�˴������޸�ϵͳ��<b>��������</b>��
t  �޸�IP��ַ�Ժ���Ҫ����������������µ�IP��ַ�Ա���������ϵͳ��<br><br>
t  	 </font></p>
t <form action=network.cgi method=post name=cgi>
t <input type=hidden value="net" name=pg>
t <table border=0 width=99%><font size="4">
t <tr bgcolor=#aaccff>
t  <th width=40%>��Ŀ</th>
t  <th width=60%>����</th></tr>
# Here begin data setting which is formatted in HTTP_CGI.C module
t <tr><td><img src=pabb.gif>�Զ����IP��ַ</td>
c a d <td><input type=hidden name=dhcp value="off" %s><input type=checkbox name=dhcp value="on" %s></td></tr>
t <tr><td><img src=pabb.gif>IP��ַ</td>
c a i <td><input type=text name=ip value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>��������</td>
c a m <td><input type=text name=msk value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>ȱʡ����</td>
c a g <td><input type=text name=gw value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>��DNS������</td>
c a p <td><input type=text name=pdns value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>��DNS������</td>
c a s <td><input type=text name=sdns value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>Զ�˷�������ַ</td>
c a r <td><input type=text name=rmts value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>Զ�˷�����UDP�˿�</td>
c a u <td><input type=text name=port value="%d" size=18 maxlength=18></td></tr>
t </font></table>
# Here begin button definitions
t <p align=center>
t <input type=button name=set value="�޸�" onclick="changeConfirm(this.form)">
t <input type=reset value="�ָ�ԭֵ">
t </p></form>
i pg_footer.inc
. End of script must be closed with period.

