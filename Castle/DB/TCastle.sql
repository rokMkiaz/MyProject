DROP Table [TCastleData]
go
DROP PROCEDURE [gp_Castle_Load_Data]
go
DROP PROCEDURE [gp_Castle_Save_Data]
go

CREATE TABLE [dbo].[TCastleData](
	[idx] [int] IDENTITY(1,1) NOT NULL,
	[castleIndex] [int] NOT NULL,
	[clanunique] [int] NULL,
	[clanname] [nvarchar](20) NULL,
	[mastername] [nvarchar](20) NULL,
	[simbol1] [int] NULL,
	[simbol2] [int] NULL,
	[simbol3] [int] NULL,
	[simbolurl] nvarchar(21) NULL,
	[relayCount] [int] NULL,
	[serverGroup] [int] NULL,
	[castleTimestate] [int] NULL,
	[beforeClanUnique] [int] NULL,
	[defenceCost] [int] NULL,
	[lastBattleTime] [datetime] NULL,
	[savetime] [datetime] NULL
) ON [PRIMARY]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_castleIndex]  DEFAULT ((0)) FOR [castleIndex]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_clanunique]  DEFAULT ((0)) FOR [clanunique]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_clanname]  DEFAULT (N'') FOR [clanname]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_mastername]  DEFAULT (N'') FOR [mastername]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_simbol1]  DEFAULT ((0)) FOR [simbol1]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_simbol2]  DEFAULT ((0)) FOR [simbol2]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_simbol3]  DEFAULT ((0)) FOR [simbol3]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_relayCount]  DEFAULT ((0)) FOR [relayCount]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_serverGroup]  DEFAULT ((0)) FOR [serverGroup]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_castleTimestate]  DEFAULT ((1)) FOR [castleTimestate]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_beforeClanUnique]  DEFAULT ((0)) FOR [beforeClanUnique]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_defenceCost]  DEFAULT ((0)) FOR [defenceCost]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_lastBattleTime]  DEFAULT ((0)) FOR [lastBattleTime]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_savetime]  DEFAULT (getdate()) FOR [savetime]
GO
ALTER TABLE [dbo].[TCastleData] ADD  CONSTRAINT [DF_TCastleData_simbolurl]  DEFAULT (N'') FOR [simbolurl]
GO



CREATE      PROCEDURE [dbo].[gp_Castle_Load_Data]

@castleIdx int,
@outCastleIdx int out,
@mastername nvarchar(20) out,
@clanunique int out,
@clanname nvarChar(20) out,
@simbol1 int out,
@simbol2 int out,
@simbol3 int out,
@simbolurl nvarChar(20) out,
@relaywincount int out,
@serverGroup int out,
@state int out,
@beclanunique int out,
@lastbattleTime datetime out,
@defenceCost int out

AS
BEGIN
	SELECT 
	@outCastleIdx = [castleIndex],
	@mastername =[mastername],
	@clanunique =[clanunique],
	@clanname = [clanname],
	@simbol1 =[simbol1],
	@simbol2 =[simbol1],
	@simbol3 =[simbol1],
	@simbolurl=[simbolurl],
	@relaywincount =[relayCount],
	@serverGroup =[serverGroup],
	@state =[castleTimestate],
	@beclanunique =[beforeClanUnique],
	@lastbattleTime =[lastBattleTime],
	@defenceCost=[defenceCost]

	FROM TCastleData WHERE castleIndex = @castleIdx

	IF( @@ROWCOUNT = 0)
	BEGIN
		INSERT INTO TCastleData(castleIndex) VALUES(@castleIdx) 
		SELECT 
		@outCastleIdx = [castleIndex],
		@mastername =[mastername],
		@clanunique =[clanunique],
		@clanname = [clanname],
		@simbol1 =[simbol1],
		@simbol2 =[simbol1],
		@simbol3 =[simbol1],
		@simbolurl=[simbolurl],
		@relaywincount =[relayCount],
		@serverGroup =[serverGroup],
		@state =[castleTimestate],
		@beclanunique =[beforeClanUnique],
		@lastbattleTime =[lastBattleTime],
		@defenceCost=[defenceCost]

		FROM TCastleData WHERE castleIndex = @castleIdx
	END
	return 0;
END

GO


CREATE PROCEDURE [dbo].[gp_Castle_Save_Data]

@castleIdx int ,

@saveCastleIdx int,
@mastername nvarchar(20),
@clanunique int,
@clanname nvarChar(20),
@simbol1 int ,
@simbol2 int ,
@simbol3 int ,
@simbolurl nvarChar(20),
@relaywincount int,
@serverGroup int ,
@state int ,
@beclanunique int,
@lastbattleTime bigint,
@defenceCost int
AS
BEGIN
	UPDATE TCastleData set 
		[mastername] = @mastername,
		[clanunique] = @clanunique,
		[clanname]  = @clanname ,
		[simbol1] =@simbol1,
		[simbol2] =@simbol2,
		[simbol3] =@simbol3,
		[simbolurl] = @simbolurl,
		[relayCount] =@relaywincount,
		[serverGroup] =@serverGroup,
		[castleTimestate] =@state,
		[beforeClanUnique] =@beclanunique,
		[lastBattleTime] = GetDate(),
		[savetime] = GetDate(),
		[defenceCost]=@defenceCost

	where castleIndex = @castleIdx
END


GO
