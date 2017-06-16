t <html><head><title>网络设置</title>
t <script language=JavaScript>
t function changeConfirm(f){
t  if(!confirm('确定要修改网络参数配置?')) return;
t  f.submit();
t }
t </script></head>
i pg_header.inc
t <h2 align=center><br>网络设置</h2>
t <p><font size="3">此处可以修改系统的<b>网络配置</b>。
t  修改IP地址以后，需要在浏览器重新输入新的IP地址以便重新连接系统。<br><br>
t  	 </font></p>
t <form action=network.cgi method=post name=cgi>
t <input type=hidden value="net" name=pg>
t <table border=0 width=99%><font size="4">
t <tr bgcolor=#aaccff>
t  <th width=40%>项目</th>
t  <th width=60%>设置</th></tr>
# Here begin data setting which is formatted in HTTP_CGI.C module
t <tr><td><img src=pabb.gif>自动获得IP地址</td>
c a d <td><input type=hidden name=dhcp value="off" %s><input type=checkbox name=dhcp value="on" %s></td></tr>
t <tr><td><img src=pabb.gif>IP地址</td>
c a i <td><input type=text name=ip value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>子网掩码</td>
c a m <td><input type=text name=msk value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>缺省网关</td>
c a g <td><input type=text name=gw value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>主DNS服务器</td>
c a p <td><input type=text name=pdns value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>副DNS服务器</td>
c a s <td><input type=text name=sdns value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>远端服务器地址</td>
c a r <td><input type=text name=rmts value="%s" size=18 maxlength=18></td></tr>
t <tr><td><img src=pabb.gif>远端服务器UDP端口</td>
c a u <td><input type=text name=port value="%d" size=18 maxlength=18></td></tr>
t </font></table>
# Here begin button definitions
t <p align=center>
t <input type=button name=set value="修改" onclick="changeConfirm(this.form)">
t <input type=reset value="恢复原值">
t </p></form>
i pg_footer.inc
. End of script must be closed with period.

